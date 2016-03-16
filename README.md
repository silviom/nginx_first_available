Description
===========

**ngx_stream_upstream_first_available_module** -  This module defines a new load balancing method for an upstream of a stream. This method uses a very easy logic. It takes the first server in the list that is available.


## Installing

#### Download nginx source
```
wget http://nginx.org/nginx-VERSION.tar.gz
tar zxvf nginx-VERSION.tar.gz
cd nginx-VERSION
```

##### To build it as a static module:
```
./configure --add-module=/path/to/nginx_first_available_folder
make
make install
```

##### To build it as a dynamic module:
```
./configure --add-dynamic-module=/path/to/nginx_first_available_folder
make
```

`make` will produce the file `objs/ngx_stream_upstream_first_available_module.so`. It should be copied to your nginx's modules folder. By last, you should add the following line to the `nginx.conf` file:
```
load_module modules/ngx_stream_upstream_first_available_module.so;
```

## Example Usage:
```
stream {
    ...
 upstream backend {
    first_available;
    server dev1:9001;
    server dev2:9001;
    server dev3:9001;
  }

  server{
    listen 9002;
    proxy_pass backend;
  }
}
```

## Copyright & License

This module is licenced under the MIT License (MIT)
Copyright (c) 2016 Silvio Massari silvio.massari@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.