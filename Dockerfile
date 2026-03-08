FROM alpine:3.14

ENV MINETEST_GAME_VERSION master

COPY .git /usr/src/multicraft/.git
COPY CMakeLists.txt /usr/src/multicraft/CMakeLists.txt
COPY README.md /usr/src/multicraft/README.md
COPY multicraft.conf.example /usr/src/multicraft/multicraft.conf.example
COPY builtin /usr/src/multicraft/builtin
COPY cmake /usr/src/multicraft/cmake
COPY doc /usr/src/multicraft/doc
COPY fonts /usr/src/multicraft/fonts
COPY lib /usr/src/multicraft/lib
COPY misc /usr/src/multicraft/misc
COPY po /usr/src/multicraft/po
COPY src /usr/src/multicraft/src
COPY textures /usr/src/multicraft/textures

WORKDIR /usr/src/multicraft

RUN apk add --no-cache git build-base irrlicht-dev cmake bzip2-dev libpng-dev \
		jpeg-dev libxxf86vm-dev mesa-dev sqlite-dev libogg-dev zstd-dev \
		libvorbis-dev openal-soft-dev curl-dev freetype-dev zlib-dev \
		gmp-dev jsoncpp-dev postgresql-dev luajit-dev ca-certificates && \
	git clone --depth=1 -b ${MINETEST_GAME_VERSION} https://github.com/minetest/minetest_game.git ./games/minetest_game && \
	rm -fr ./games/minetest_game/.git

WORKDIR /usr/src/
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp/ && \
	mkdir prometheus-cpp/build && \
	cd prometheus-cpp/build && \
	cmake .. \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DENABLE_TESTING=0 && \
	make -j2 && \
	make install

WORKDIR /usr/src/multicraft
RUN mkdir build && \
	cd build && \
	cmake .. \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SERVER=TRUE \
		-DENABLE_PROMETHEUS=TRUE \
		-DBUILD_UNITTESTS=FALSE \
		-DBUILD_CLIENT=FALSE && \
	make -j2 && \
	make install

FROM alpine:3.14

RUN apk add --no-cache sqlite-libs curl gmp libstdc++ libgcc libpq luajit && \
	adduser -D multicraft --uid 30000 -h /var/lib/multicraft && \
	chown -R multicraft:multicraft /var/lib/multicraft

WORKDIR /var/lib/multicraft

COPY --from=0 /usr/local/share/multicraft /usr/local/share/multicraft
COPY --from=0 /usr/local/bin/multicraftserver /usr/local/bin/multicraftserver
COPY --from=0 /usr/local/share/doc/multicraft/multicraft.conf.example /etc/multicraft/multicraft.conf

USER multicraft:multicraft

EXPOSE 30000/udp 30000/tcp

CMD ["/usr/local/bin/multicraftserver", "--config", "/etc/multicraft/multicraft.conf"]
