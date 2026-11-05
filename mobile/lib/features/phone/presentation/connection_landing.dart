import 'package:flutter/material.dart';

import '../../../app/theme/companion_theme.dart';
import '../../../app/widgets/bike_hero.dart';

/// First connection presentation; connection and trust stay with the session.
class ConnectionLanding extends StatelessWidget {
  const ConnectionLanding({
    required this.onConnect,
    this.error,
    this.busy = false,
    super.key,
  });
  final VoidCallback onConnect;
  final String? error;
  final bool busy;

  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    final text = Theme.of(context).textTheme;
    return Scaffold(
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(
              maxWidth: CompanionTokens.contentWidth,
            ),
            child: CustomScrollView(
              slivers: [
                SliverFillRemaining(
                  hasScrollBody: false,
                  child: Padding(
                    padding: const EdgeInsets.all(CompanionTokens.pagePadding),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        Text(
                          'R15',
                          style: text.headlineMedium?.copyWith(
                            fontFamily: 'ChakraPetch',
                            fontWeight: FontWeight.w700,
                            fontStyle: FontStyle.italic,
                          ),
                        ),
                        Text(
                          'C O M P A N I O N',
                          style: text.labelSmall?.copyWith(
                            color: colors.onSurfaceVariant,
                          ),
                        ),
                        const SizedBox(height: 12),
                        SizedBox(
                          height: (MediaQuery.sizeOf(context).height * 0.40)
                              .clamp(200.0, 380.0),
                          child: Stack(
                            alignment: Alignment.center,
                            children: [
                              Positioned.fill(
                                child: ExcludeSemantics(
                                  child: CustomPaint(
                                    painter: _ClusterBackdrop(colors.primary),
                                  ),
                                ),
                              ),
                              const BikeHero(),
                            ],
                          ),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          'Your bike.',
                          style: text.displaySmall?.copyWith(
                            fontWeight: FontWeight.w600,
                            letterSpacing: -1.2,
                          ),
                        ),
                        Text(
                          'In sync.',
                          style: text.displaySmall?.copyWith(
                            fontWeight: FontWeight.w600,
                            letterSpacing: -1.2,
                            color: colors.primary,
                          ),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'Pair your smart cluster to see supported bike readings and sync your phone.',
                          style: text.bodyLarge?.copyWith(
                            color: colors.onSurfaceVariant,
                            height: 1.5,
                          ),
                        ),
                        const SizedBox(height: 24),
                        const Spacer(),
                        if (error != null) ...[
                          Semantics(
                            liveRegion: true,
                            child: Text(
                              error!,
                              style: TextStyle(color: colors.error),
                            ),
                          ),
                          const SizedBox(height: 16),
                        ],
                        FilledButton.icon(
                          onPressed: busy ? null : onConnect,
                          icon: const Icon(Icons.bluetooth),
                          label: Text(
                            busy
                                ? 'Connecting…'
                                : error != null
                                ? 'Retry connection'
                                : 'Connect cluster',
                          ),
                        ),
                        const SizedBox(height: 16),
                        Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(
                              Icons.power_settings_new,
                              size: 18,
                              color: colors.onSurfaceVariant,
                            ),
                            const SizedBox(width: 10),
                            Flexible(
                              child: Text(
                                'Keep your cluster powered on and nearby.',
                                style: text.bodySmall?.copyWith(
                                  color: colors.onSurfaceVariant,
                                ),
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _ClusterBackdrop extends CustomPainter {
  const _ClusterBackdrop(this.color);
  final Color color;
  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = color.withValues(alpha: 0.22)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;
    canvas.drawCircle(
      Offset(size.width / 2, size.height * 0.44),
      size.shortestSide * 0.43,
      paint,
    );
    final path = Path()
      ..moveTo(size.width * .04, size.height * .43)
      ..lineTo(size.width * .04, size.height * .22)
      ..lineTo(size.width * .12, size.height * .15)
      ..lineTo(size.width * .40, size.height * .15)
      ..lineTo(size.width * .44, size.height * .09)
      ..lineTo(size.width * .79, size.height * .09)
      ..lineTo(size.width * .84, size.height * .15)
      ..lineTo(size.width * .96, size.height * .15)
      ..lineTo(size.width * .96, size.height * .43)
      ..close();
    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(_ClusterBackdrop oldDelegate) =>
      oldDelegate.color != color;
}
