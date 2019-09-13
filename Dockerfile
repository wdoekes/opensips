ARG oscodename=stretch
FROM debian:${oscodename}
MAINTAINER Walter Doekes <wjdoekes+opensips@osso.nl>

ARG DEBIAN_FRONTEND=noninteractive

# This time no "keeping the build small". We only use this container for
# building/testing and not for running, so we can keep files like apt
# cache.
RUN echo 'APT::Install-Recommends "0";' >/etc/apt/apt.conf.d/01norecommends
RUN apt-get update -q
RUN apt-get dist-upgrade -y
RUN apt-get install -y \
    bzip2 ca-certificates curl git \
    build-essential dh-autoreconf devscripts dpkg-dev equivs quilt

# Copy source, install dependencies.
COPY . /build/opensips
WORKDIR /build/opensips
RUN sed -i -e "1s/<VERSION>/0vErSiOn/" packaging/debian/changelog && \
    sed -i -e "1s/ <RELEASE>;/ stable;/" packaging/debian/changelog && \
    mk-build-deps --install --remove --tool "apt-get -y" \
        packaging/debian/control && \
    sed -i -e "1s/0vErSiOn/<VERSION>/" packaging/debian/changelog && \
    sed -i -e "1s/ stable;/ <RELEASE>;/" packaging/debian/changelog

# Set version and make packages.
ARG changelogversion=
ARG oscodename=stretch
RUN test -n "${changelogversion}" && \
    sed -i -e "s/<VERSION>/${changelogversion}/" packaging/debian/changelog && \
    sed -i -e "s/<RELEASE>/${oscodename}/" packaging/debian/changelog
RUN make deb

# Copy version to output dir.
CMD name=$(find /build -maxdepth 1 -name '*.dsc' | sed -e 's#^/build/##;s/.dsc$//') && \
    rm -rf /build/output/$name && mkdir /build/output/$name && \
    find /build/ -maxdepth 1 -type f -print0 | xargs -0 -IX mv X /build/output/$name/ && \
    ls -l /build/output/$name/ && \
    echo && echo 'Output files created succesfully'

#RUN rm -rf debian/.cache debian/*.sig
#
#RUN mv /build/pigeonhole.patch debian/patches/pigeonhole.patch
#
## Build, but become 'bin' before we do, because the next postfix tests
## refuse to run as 'root'. And yes, we chown '..' because
## dpkg-buildpackage touches some files there too.
#RUN chown -R bin ..
#USER bin
#RUN DEB_BUILD_OPTIONS=parallel=6 dpkg-buildpackage -us -uc -sa
#USER root
#
## TODO: for bonus points, we could run quick tests here;
## for starters dpkg -i tests?
#
## Write output files (store build args in ENV first).
#ENV oscodename=$oscodename \
#    upname=$upname upversion=$upversion debversion=$debversion
#CMD . /etc/os-release && fullversion=${upversion}-${debversion}+${ID%%[be]*}${VERSION_ID} && \
#    dist=Docker.out && \
#    if ! test -d "/${dist}"; then echo "Please mount ./${dist} for output" >&2; false; fi && \
#    echo && . /etc/os-release && mkdir "/${dist}/${oscodename}/${upname}_${fullversion}" && \
#    mv /build/*${fullversion}* "/${dist}/${oscodename}/${upname}_${fullversion}/" && \
#    mv /build/${upname}_${upversion}.orig.tar.gz "/${dist}/${oscodename}/${upname}_${fullversion}/" && \
#    chown -R ${UID}:root "/${dist}/${oscodename}" && \
#    cd / && find "${dist}/${oscodename}/${upname}_${fullversion}" -type f && \
