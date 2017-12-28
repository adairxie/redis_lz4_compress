# redis_lz4_compress
基于redis 4.0以上版本开发的redis 压缩响应数据模块

## Example usage

Simple redis string compression/decompression
```redis
127.0.0.1:6379> set test "hello"
OK
127.0.0.1:6379> get test
"hello"
127.0.0.1:6379> lz4.compress test
"\x04\"M\x18`@\x82\x05\x00\x80hello\x00\x00\x00\x00"
127.0.0.1:6379> get test
"hello"
127.0.0.1:6379>
```

## Build/Install Instructions

Require lz4 libarary
```
 gcc -c -Wall -Werror -fPIC lz4_module.c -llz4
 gcc -shared -o lz4_module.so lz4_module.o -llz4
```
