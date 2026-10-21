import 'package:flutter/widgets.dart';

import 'app/companion_app.dart';
import 'features/phone/domain/phone_status.dart';

void main() {
  runApp(const CompanionApp(phoneStatus: PhoneStatus.foundation()));
}
