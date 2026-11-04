/// Immutable native navigation observations; independent of Flutter and BLE DTOs.
class NavigationState {
  NavigationState(Map<dynamic, dynamic>? value)
    : data = Map<String, dynamic>.unmodifiable(value ?? const {});
  final Map<String, dynamic> data;
  bool flag(String key) => data[key] == true;
  String? text(String key) => data[key] is String ? data[key] as String : null;
  int get mapRevision => (data['mapRevision'] as num?)?.toInt() ?? -1;
  Map<String, dynamic> part(String key) => data[key] is Map
      ? Map<String, dynamic>.unmodifiable(data[key] as Map)
      : const {};
  Map<String, dynamic> get location => part('location');
  Map<String, dynamic> get route => part('route');
  Map<String, dynamic> get download => part('download');
  List<Map<String, dynamic>> get suggestions =>
      (data['suggestions'] as List? ?? const [])
          .whereType<Map<dynamic, dynamic>>()
          .map((item) => Map<String, dynamic>.unmodifiable(item))
          .toList(growable: false);
}
