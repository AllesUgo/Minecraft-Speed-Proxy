FROM alpine:latest AS Builder

WORKDIR /src
RUN apk add build-base gcc g++ make git libexecinfo-dev
RUN git clone https://github.com/AllesUgo/Minecraft-Speed-Proxy
RUN cd Minecraft-Speed-Proxy && make && make install

FROM alpine:latest
EXPOSE 25565

COPY --from=Builder /usr/bin/minecraftspeedproxy /bin/
ENTRYPOINT [ "/bin/minecraftspeedproxy" ]