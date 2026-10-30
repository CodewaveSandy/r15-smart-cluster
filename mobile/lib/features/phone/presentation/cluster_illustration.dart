import 'package:flutter/material.dart';

/// Decorative rectangular HMI silhouette, without invented live readings.
class ClusterIllustration extends StatelessWidget {
  const ClusterIllustration({super.key});
  @override
  Widget build(BuildContext context) {
    final colors = Theme.of(context).colorScheme;
    return ExcludeSemantics(
      child: Center(
        child: SizedBox(
          width: 210,
          height: 126,
          child: DecoratedBox(
            decoration: BoxDecoration(
              color: colors.surface,
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: colors.outlineVariant, width: 3),
            ),
            child: Padding(
              padding: const EdgeInsets.all(10),
              child: DecoratedBox(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(
                    color: colors.primary.withValues(alpha: .5),
                  ),
                ),
                child: Center(
                  child: Icon(
                    Icons.speed_outlined,
                    color: colors.primary,
                    size: 58,
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
