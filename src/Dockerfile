FROM ubuntu:18.04
RUN mkdir /var/log/erss
RUN apt-get update && apt-get -y install g++ make
ADD ./src /var/log/erss
ADD proxy.log /var/log/erss
WORKDIR /var/log/erss/src

CMD ["./http_caching_proxy"]