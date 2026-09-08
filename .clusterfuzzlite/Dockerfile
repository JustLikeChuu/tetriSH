# google's prebuilt image with clang, libFuzzer, and all the sanitizers already installed 
FROM gcr.io/oss-fuzz-base/base-builder:v1 
# copy the whole repo in at build time -> the main difference from full OSS-Fuzz, which does a live git clone instead; CFLite just snapshots whatever is present
COPY . $SRC/tetriSH
# places you where build.sh is expected to run from; the repo root
WORKDIR $SRC/tetriSH
# stages the build scropt into a predictable location the harness runner knows to execute
COPY ./.clusterfuzzlite/build.sh $SRC/