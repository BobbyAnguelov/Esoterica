FROM ufbx.users.base

WORKDIR /work

COPY 001-clone.sh .
RUN bash 001-clone.sh

ARG CORRADE_SHA=master
ENV CORRADE_SHA=$CORRADE_SHA
ARG MAGNUM_SHA=master
ENV MAGNUM_SHA=$MAGNUM_SHA
COPY 002-compile-magnum.sh .
RUN bash 002-compile-magnum.sh

ARG MAGNUM_PLUGINS_SHA=master
ENV MAGNUM_PLUGINS_SHA=$MAGNUM_PLUGINS_SHA
COPY 003-compile-plugins.sh .
RUN bash 003-compile-plugins.sh

COPY 999-execute.sh .
CMD ["bash", "999-execute.sh"]
