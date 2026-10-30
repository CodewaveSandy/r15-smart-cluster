import 'package:flutter/material.dart';

/// Local, decorative artwork; contains no telemetry or service state.
class BikeHero extends StatelessWidget {
  const BikeHero({super.key});

  @override
  Widget build(BuildContext context) => Image.asset(
    'assets/illustrations/bike-opening.png',
    fit: BoxFit.contain,
    // Bound decoded memory even on large phones; retain a sharp 2x asset.
    cacheWidth: 900,
    excludeFromSemantics: true,
    errorBuilder: (context, error, stack) => Icon(
      Icons.two_wheeler_outlined,
      size: 120,
      color: Theme.of(context).colorScheme.primary,
    ),
  );
}
