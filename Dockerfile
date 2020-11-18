FROM zephyrprojectrtos/zephyr-build:latest

ENTRYPOINT ["/bin/bash"]
CMD ["/bin/bash"]

COPY . /workdir

USER root
RUN chown -R user:user /workdir

# ENV ZEPHYR_BASE=/workdir
# RUN west update
# RUN export ZEPHYR_BASE=`pwd` && make app-QSIB

