FROM debian:stable AS Builder

WORKDIR /src
RUN apt update -y
RUN apt install build-essential gcc g++ make git -y
RUN git clone https://github.com/AllesUgo/Minecraft-Speed-Proxy
RUN cd Minecraft-Speed-Proxy && make USE_SYSTEM_LIBM=1 && make install

FROM debian:stable
EXPOSE 25565

COPY --from=Builder /usr/bin/minecraftspeedproxy /usr/bin
ENTRYPOINT [ "/usr/bin/minecraftspeedproxy" ]