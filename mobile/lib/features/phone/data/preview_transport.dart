import 'dart:convert';
import 'dart:io';

abstract interface class PreviewTransport {
  Future<Map<String, dynamic>> request(
    String operation,
    Map<String, Object?> data,
  );
}

class PreviewFailure implements Exception {
  const PreviewFailure(this.message, {this.code});
  final String? code;
  final String message;
}

/// Debug-only TLS client. Trusts only the certificate supplied by the launcher.
final class TlsPreviewTransport implements PreviewTransport {
  TlsPreviewTransport({
    required this.host,
    this.port = 8765,
    required this.token,
    required this.certificate,
  }) {
    _context.setTrustedCertificatesBytes(base64Decode(certificate));
  }
  final String certificate;
  final String host;
  TlsPreviewTransport atEndpoint(String host, int port) => TlsPreviewTransport(
    host: host,
    port: port,
    token: token,
    certificate: certificate,
  );
  final int port;
  final String token;
  final _context = SecurityContext(withTrustedRoots: false);

  @override
  Future<Map<String, dynamic>> request(
    String operation,
    Map<String, Object?> data,
  ) async {
    final client = HttpClient(context: _context)
      ..connectionTimeout = const Duration(seconds: 3);
    try {
      return await _send(
        client,
        operation,
        data,
      ).timeout(const Duration(seconds: 4));
    } finally {
      client.close(force: true);
    }
  }

  Future<Map<String, dynamic>> _send(
    HttpClient client,
    String operation,
    Map<String, Object?> data,
  ) async {
    final request = await client.postUrl(
      Uri(scheme: 'https', host: host, port: port, path: operation),
    );
    request.followRedirects = false;
    request.headers.set(HttpHeaders.authorizationHeader, token);
    request.headers.contentType = ContentType.json;
    final bytes = utf8.encode(jsonEncode({'v': 1, ...data}));
    request.contentLength = bytes.length;
    request.add(bytes);
    final response = await request.close();
    final body = <int>[];
    await for (final chunk in response) {
      if (body.length + chunk.length > 2048) {
        throw const PreviewFailure('Invalid preview response.');
      }
      body.addAll(chunk);
    }
    final decoded = jsonDecode(utf8.decode(body));
    if (decoded is! Map<String, dynamic>) {
      throw const PreviewFailure('Invalid preview response.');
    }
    if (response.statusCode != 200) {
      throw PreviewFailure(switch (decoded['error']) {
        'approval_required' => 'Allow pairing on your cluster to continue.',
        'incorrect_code' =>
          'The pairing code changed. Search again and check the cluster.',
        'enrollment_expired' =>
          'On the cluster, open Phone and select Pair phone, then try again.',
        'already_paired' => 'This preview already has a paired app. Restart the Mac preview to pair again.',
        'preview_unavailable' =>
          'The cluster preview is not ready. Wait for its window to open.',
        'session_expired' =>
          'This pairing was removed. Connect and pair again.',
        'storage_unavailable' =>
          'The cluster could not save pairing. Try again.',
        _ => 'Preview rejected the connection. Restart both previews.',
      }, code: decoded['error'] as String?);
    }
    return decoded;
  }
}
