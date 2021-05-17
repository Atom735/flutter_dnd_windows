import 'package:flutter/material.dart';
import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_dnd_windows/flutter_dnd_windows.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Center(
          child: StreamBuilder(
            stream: FlutterDndWindowsPlugin().receiveBroadcastStream(),
            initialData: {},
            builder: (context, snapshot) {
              if (snapshot.hasData) {
                final data = (snapshot.data as Map).cast<String, Object>();
                if (data['type'] == 'enter' || data['type'] == 'over') {
                  return Column(
                    children: [
                      Text('Курсор: ${data['x']} x ${data['y']}'),
                      Text('Перемещаются сюда след данные'),
                      ...(data['items'] as List)
                          .cast<String>()
                          .map((e) => Text(e)),
                    ],
                  );
                }
                if (data['type'] == 'drop') {
                  return Column(
                    children: [
                      Text('Курсор: ${data['x']} x ${data['y']}'),
                      Text('Данные дропнуты сюда'),
                      ...(data['items'] as List)
                          .cast<String>()
                          .map((e) => Text(e)),
                    ],
                  );
                }
              }
              return Text('Переместите сюда файлы');
            },
          ),
        ),
      ),
    );
  }
}
