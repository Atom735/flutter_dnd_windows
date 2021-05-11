
import 'dart:async';

import 'package:flutter/services.dart';

class FlutterDndWindows {
  static const MethodChannel _channel =
      const MethodChannel('flutter_dnd_windows');

  static Future<String> get platformVersion async {
    final String version = await _channel.invokeMethod('getPlatformVersion');
    return version;
  }
}
