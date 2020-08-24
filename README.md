# 30daysOS

```
$ sudo apt install gcc nasm qemu-system-x86 qemu-kvm libvirt-bin
```

Ubuntu 18.04/20.04のQEMU上で動作することを確認している。
コンパイルの方法などはMakefileを参照。

## Ubuntu 20.04環境でのトラブルシューティング

### ldで.note.gnu.propertyセクションと.dataセクションが重なる

14日目終了時に20.04環境でmakeしたところ
```
/usr/bin/ld: section .note.gnu.property LMA [0000000000002c1c,0000000000002d6b] overlaps section .data LMA [0000000000002c19,0000000000003ed4]
```
というエラーが出たので、har.ldのdataセクションのアドレスを以下のようにずらすことで動作させた。
```
.data 0x310000 : AT ( ADDR(.text) + SIZEOF(.text) + 0x200 ) {
```
ちょっと無理やりな解決法な気がするが、動いているので良しとする

## 8日目

- マウス移動時の数値表示がおかしくなっているが、うまくうごいているので無視した

## 9日目

- メモリテストがアセンブラで書く前から正しい値を返してきた
  - `-O2`でコンパイルすると本の通り
  - QEMUの初期設定が128MBになっていることに注意([リンク](https://wiki.gentoo.org/wiki/QEMU/Options#RAM))
    - `-m 32`をつけてやることで32MBにできる

## 13日目

- for文の直下を`count++`のみにすると動作しなくなった
  - [このブログ](https://wisteria0410ss.hatenablog.com/entry/2019/02/10/222931)を参考に`-enable-kvm`をパラメータに追加
  - permission deniedが出たので、[ここ](https://canal.idletime.be/qemu/ubuntu.html)を参考に`$ gpasswd -a (user) kvm`

## 14日目
- 解像度設定で`0x107`は対応していないと書いてあるが、動作した

## 17日目

- 使っているキーボードの関係でLock系のキーが正しく動いているのかわからない

## 20日目
- プロセッサが賢いのか、`RETF`にしなくても動作した

