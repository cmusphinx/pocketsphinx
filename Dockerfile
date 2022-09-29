FROM alpine:latest as runtime
RUN apk add --no-cache python3 py3-pip sox

FROM runtime as build
RUN apk add --no-cache cmake ninja gcc musl-dev python3-dev pkgconfig

COPY . /pocketsphinx
WORKDIR /pocketsphinx
RUN cmake -S . -B build -DBUILD_SHARED_LIBS=ON -G Ninja && cmake --build build --target install
# Cannot use --build-option because pip sucks
RUN CMAKE_ARGS="-DUSE_INSTALLED_POCKETSPHINX=ON" pip wheel -v .

FROM runtime
COPY --from=build /usr/local/ /usr/local/
COPY --from=build /pocketsphinx/*.whl /
RUN pip install /*.whl && rm /*.whl

RUN adduser -u 1000 -DHD pocketsphinx && adduser pocketsphinx audio
COPY examples/ /work/examples/
RUN chown -R pocketsphinx:pocketsphinx /work
USER pocketsphinx
WORKDIR /work
