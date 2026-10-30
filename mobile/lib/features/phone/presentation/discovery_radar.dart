import 'dart:math' as math;

import 'package:flutter/material.dart';

/// Exists only during a real scan. Decorative sweep never invents devices.
class DiscoveryRadar extends StatefulWidget {
  const DiscoveryRadar({this.scanning = true, this.diameter = 250, super.key});
  final bool scanning;
  final double diameter;
  @override
  State<DiscoveryRadar> createState() => _DiscoveryRadarState();
}

class _DiscoveryRadarState extends State<DiscoveryRadar>
    with SingleTickerProviderStateMixin {
  late final AnimationController _sweep = AnimationController(
    vsync: this,
    duration: const Duration(milliseconds: 2400),
  );
  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    _sync();
  }

  @override
  void didUpdateWidget(DiscoveryRadar oldWidget) {
    super.didUpdateWidget(oldWidget);
    _sync();
  }

  void _sync() {
    if (!widget.scanning ||
        MediaQuery.disableAnimationsOf(context) ||
        !TickerMode.valuesOf(context).enabled) {
      _sweep.stop();
    } else if (!_sweep.isAnimating) {
      _sweep.repeat();
    }
  }

  @override
  void dispose() {
    _sweep.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => ExcludeSemantics(
    child: RepaintBoundary(
      child: SizedBox.square(
        dimension: widget.diameter,
        child: AnimatedBuilder(
          animation: _sweep,
          builder: (context, child) => CustomPaint(
            painter: _RadarPainter(
              Theme.of(context).colorScheme.primary,
              _sweep.value,
            ),
          ),
        ),
      ),
    ),
  );
}

class _RadarPainter extends CustomPainter {
  const _RadarPainter(this.color, this.progress);
  final Color color;
  final double progress;
  @override
  void paint(Canvas canvas, Size size) {
    final center = size.center(Offset.zero);
    final radius = size.shortestSide / 2 - 2;
    final bounds = Rect.fromCircle(center: center, radius: radius);
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1
      ..color = color.withValues(alpha: .45);
    for (final fraction in [.33, .66, 1.0]) {
      canvas.drawCircle(center, radius * fraction, paint);
    }
    canvas.save();
    canvas.translate(center.dx, center.dy);
    canvas.rotate(progress * math.pi * 2);
    canvas.translate(-center.dx, -center.dy);
    canvas.drawArc(
      bounds,
      -math.pi / 2,
      math.pi / 3,
      true,
      Paint()
        ..shader = SweepGradient(
          startAngle: -math.pi / 2,
          endAngle: -math.pi / 6,
          colors: [color.withValues(alpha: 0), color.withValues(alpha: .26)],
          transform: const GradientRotation(-math.pi / 2),
        ).createShader(bounds),
    );
    canvas.restore();
    canvas.drawCircle(center, 5, Paint()..color = color);
  }

  @override
  bool shouldRepaint(_RadarPainter oldDelegate) =>
      color != oldDelegate.color || progress != oldDelegate.progress;
}
