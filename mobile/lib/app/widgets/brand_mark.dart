import 'package:flutter/material.dart';

/// Typographic project mark; no external logo or trademark asset required.
class BrandMark extends StatelessWidget {
  const BrandMark({super.key});

  @override
  Widget build(BuildContext context) => Semantics(
    label: 'R15 Companion',
    child: ExcludeSemantics(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            'R15',
            style: Theme.of(context).textTheme.displayLarge?.copyWith(
              fontWeight: FontWeight.w900,
              fontStyle: FontStyle.italic,
              letterSpacing: -3,
              color: Theme.of(context).colorScheme.primary,
            ),
          ),
          const Text('C O M P A N I O N'),
        ],
      ),
    ),
  );
}
