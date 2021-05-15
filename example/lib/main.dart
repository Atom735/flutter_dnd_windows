import 'package:flutter/material.dart';
import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_dnd_windows/flutter_dnd_windows.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatefulWidget {
  @override
  _MyAppState createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String _platformVersion = 'Unknown';

  final _plugin = FlutterDndWindowsPlugin();
  late StreamSubscription _ss;
  final _ssVal = ValueNotifier<StreamSubscription?>(null);

  @override
  void initState() {
    super.initState();
    _ss = _plugin
        .receiveBroadcastStream('Some argumets: Hello From Flutter!')
        .listen(print);
  }

  @override
  dispose() {
    if (_ssVal.value != null) {
      _ssVal.value!.cancel();
    }
    _ss.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Center(
          child: ValueListenableBuilder(
            valueListenable: _ssVal,
            builder: (context, val, child) => TextButton(
              onPressed: () {
                setState(() {
                  if (_ssVal.value == null) {
                    _ssVal.value = _plugin
                        .receiveBroadcastStream()
                        .listen((s) => print('A: $s'));
                  } else {
                    _ssVal.value!.cancel();
                    _ssVal.value = null;
                  }
                });
              },
              child: val == null
                  ? Text('Preess for create stream')
                  : Text('Preess for destroy stream'),
            ),
          ),
        ),
      ),
    );
  }
}
