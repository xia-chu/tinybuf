# 一个高效的序列化反序列化库

- [x] 支持json序列化反序列化
- [x] 支持tinybuf([私有协议](https://github.com/xiongziliang/tinybuf/wiki/tinybuf%E5%BA%8F%E5%88%97%E5%8C%96%E5%8D%8F%E8%AE%AE))序列化反序列化
- [x] 纯C代码，适合嵌入式系统
- [x] 序列化反序列化性能高效
- [x] tinybuf序列化格式占用字节更少，更利于传输

# 性能对比(release模式)
- [x] json格式时，序列化性能大约为jsoncpp的3倍
- [x] json格式时，反序列化性能大约为jsoncpp的1.5倍
- [x] tinybuf格式时，序列化性能大约为jsoncpp的10~20倍
- [x] tinybuf格式时，反序列化性能大约为jsoncpp的4倍

# 测试结果截图
![image](https://user-images.githubusercontent.com/11495632/65878363-8076b500-e3c0-11e9-807a-c2ef51efe11c.png)
