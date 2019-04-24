ARG oscodename=stretch
FROM debian:${oscodename}

ENV DEBIAN_FRONTEND noninteractive

RUN sed -i -e 's#http://\([^/]*\)/\?#http://apt.osso.nl/#' /etc/apt/sources.list
RUN apt-get update
RUN apt-get install --yes --no-install-recommends apt-utils
RUN apt-get install --yes --no-install-recommends \
    build-essential ca-certificates devscripts equivs fakeroot git \
    init-system-helpers ssl-cert

ARG oscodename=stretch
ARG upversion=2.4.5
ARG debepoch=
ARG debversion=-1

RUN echo "About to build opensips-$upversion ($debversion)" >&2
ADD . /usr/src/opensips-$upversion
WORKDIR /usr/src/opensips-$upversion
RUN sed -i -e "1s#-[0-9]*<VERSION>#-$debversion#" packaging/debian/changelog

# Install build dependencies based on control file.
RUN mk-build-deps --install --remove \
    --tool='apt-get --no-install-recommends --yes' \
    packaging/debian/control

# Save a source tar-gz and make the deb packages.
RUN make proper && \
    tar -czf /usr/src/opensips_$upversion.orig.tar.gz \
      -C /usr/src opensips-$upversion
RUN make deb
