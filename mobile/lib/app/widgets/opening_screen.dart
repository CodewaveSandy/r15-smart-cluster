import 'package:flutter/material.dart';

import '../theme/companion_theme.dart';
import 'bike_hero.dart';

/// Presentation only: the app's branding deadline owns when this is removed.
class OpeningScreen extends StatefulWidget {
  const OpeningScreen({super.key});

  @override
  State<OpeningScreen> createState() => _OpeningScreenState();
}

class _OpeningScreenState extends State<OpeningScreen>
    with SingleTickerProviderStateMixin {
  late final AnimationController _pulse = AnimationController(
    vsync: this,
    duration: CompanionTokens.pulseDuration,
  );

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (MediaQuery.disableAnimationsOf(context) ||
        !TickerMode.valuesOf(context).enabled) {
      _pulse.stop();
      _pulse.value = 0;
    } else if (!_pulse.isAnimating) {
      _pulse.repeat(reverse: true);
    }
  }

  @override
  void dispose() {
    _pulse.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    return Scaffold(
      body: SafeArea(
        child: Center(
          child: ConstrainedBox(
            constraints: const BoxConstraints(
              maxWidth: CompanionTokens.contentWidth,
            ),
            child: LayoutBuilder(
              builder: (context, constraints) => SingleChildScrollView(
                child: ConstrainedBox(
                  constraints: BoxConstraints(minHeight: constraints.maxHeight),
                  child: Padding(
                    padding: const EdgeInsets.all(CompanionTokens.pagePadding),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text('R15', style: _brand(context, 32)),
                        const SizedBox(height: 20),
                        SizedBox(
                          height: (constraints.maxHeight * 0.53).clamp(
                            200,
                            480,
                          ),
                          child: Stack(
                            alignment: Alignment.center,
                            children: [
                              Positioned.fill(
                                child: RepaintBoundary(
                                  child: AnimatedBuilder(
                                    animation: _pulse,
                                    builder: (context, child) => Center(
                                      child: FractionallySizedBox(
                                        widthFactor: 0.82 + _pulse.value * 0.06,
                                        child: AspectRatio(
                                          aspectRatio: 1,
                                          child: DecoratedBox(
                                            decoration: BoxDecoration(
                                              shape: BoxShape.circle,
                                              border: Border.all(
                                                color: colors.primary
                                                    .withValues(
                                                      alpha:
                                                          0.28 +
                                                          _pulse.value * 0.2,
                                                    ),
                                              ),
                                              gradient: RadialGradient(
                                                colors: [
                                                  colors.primary.withValues(
                                                    alpha: 0.13,
                                                  ),
                                                  colors.primary.withValues(
                                                    alpha: 0,
                                                  ),
                                                ],
                                              ),
                                            ),
                                          ),
                                        ),
                                      ),
                                    ),
                                  ),
                                ),
                              ),
                              const RepaintBoundary(child: BikeHero()),
                            ],
                          ),
                        ),
                        const SizedBox(height: 28),
                        Text('YOUR RIDE.', style: _brand(context, 40)),
                        Text(
                          'IN SYNC.',
                          style: _brand(
                            context,
                            40,
                          ).copyWith(color: colors.primary),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'R15 Companion',
                          style: Theme.of(context).textTheme.titleMedium
                              ?.copyWith(
                                letterSpacing: 2.5,
                                color: colors.onSurfaceVariant,
                              ),
                        ),
                        const SizedBox(height: 28),
                        SizedBox(
                          width: 24,
                          child: Divider(color: colors.primary, thickness: 2),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'A smarter connection\nfor every ride.',
                          style: Theme.of(context).textTheme.bodyMedium
                              ?.copyWith(
                                color: colors.onSurfaceVariant,
                                height: 1.5,
                              ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  TextStyle _brand(BuildContext context, double size) => TextStyle(
    fontFamily: 'ChakraPetch',
    fontWeight: FontWeight.w700,
    fontStyle: FontStyle.italic,
    fontSize: size,
    height: 1.05,
    letterSpacing: -0.8,
    color: Theme.of(context).colorScheme.onSurface,
  );
}
