import 'dart:async';

import 'package:flutter/gestures.dart';
import 'package:flutter/services.dart';

class FlutterDndWindowsPlugin extends EventChannel {
  FlutterDndWindowsPlugin._() : super('flutter_dnd_windows');
  static final _instance = FlutterDndWindowsPlugin._();
  factory FlutterDndWindowsPlugin() => _instance;

  Stream<String>? _stream;

  Stream<String> receiveBroadcastStream([args]) =>
      _stream ??= super.receiveBroadcastStream().cast<String>();
}
