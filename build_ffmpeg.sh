mkdir ./ffmpeg

sudo apt install \
  autoconf \
  automake \
  build-essential \
  cmake \
  git \
  libass-dev \
  libfreetype6-dev \
  libsdl2-dev \
  libtheora-dev \
  libtool \
  libva-dev \
  libvdpau-dev \
  libvorbis-dev \
  libxcb1-dev \
  libxcb-shm0-dev \
  libxcb-xfixes0-dev \
  mercurial \
  pkg-config \
  texinfo \
  wget \
  zlib1g-dev \
  nasm \
  yasm \
  libvpx-dev \
  libopus-dev \
  libx264-dev \
  libmp3lame-dev \
  libfdk-aac-dev \
  libaom-dev \
  libx265-dev
  
wget -O ffmpeg-5.1.tar.bz2 https://ffmpeg.org/releases/ffmpeg-5.1.tar.bz2 && \
  tar xjvf ffmpeg-5.1.tar.bz2 && \
  cd ffmpeg-5.1 && \
  PKG_CONFIG_PATH="../ffmpeg/lib/pkgconfig" ./configure \
    --prefix="../ffmpeg" \
    --extra-libs="-lpthread -lm" \
    --bindir="../ffmpeg" \
    --enable-gpl \
    --enable-libass \
    --enable-libfdk-aac \
    --enable-libmp3lame \
    --enable-libx264 \
    --enable-libx265 \
    --enable-libtheora \
    --enable-libfreetype \
    --enable-libvorbis \
    --enable-libopus \
    --enable-libvpx \
    --enable-libaom \
    --enable-nonfree && \
  make && \
  make install && \
  hash -r
