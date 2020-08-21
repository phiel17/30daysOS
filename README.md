# 30daysOS

```
$ sudo apt install gcc nasm qemu-system-x86
```

Ubuntu 18.04/20.04のQEMU上で動作することを確認している。
コンパイルの方法などはMakefileを参照。

## 8日目

- マウス移動時の数値表示がおかしくなっているが、うまくうごいているので無視した

## 9日目

- メモリテストがアセンブラで書く前から正しい値を返してきた
  - `-O2`でコンパイルすると本の通り
  - QEMUの初期設定が128MBになっていることに注意([リンク](https://wiki.gentoo.org/wiki/QEMU/Options#RAM))
    - `-m 32`をつけてやることで32MBにできる