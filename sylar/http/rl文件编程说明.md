- 
  - http11_common.h 为http协议解析的头文件

  下面四个文件为http协议解析的实现文件
  - http11_parser.h 
  - http11_parser.rl 

  - httpclient_parser.h
  - httpclient_parser.rl

- 
  - $ ragel -v			// 获取版本号
  - $ ragel -help		// 获取帮助文档

  - $ ragel -G2 -C http11_parser.rl -o http11_parser.cc
  - $ ragel -G2 -C httpclient_parser.rl -o httpclient_parser.cc
  - $ mv http11_parser.cc http11_parser.rl.cc			        // 与自己写的代码区分开
  - $ mv httpclient_parser.cc httpclient_parser.rl.cc     // 与自己写的代码区分开