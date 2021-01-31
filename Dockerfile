FROM gcc:4.9 AS gcc
RUN apt-get update && apt-get install -y --no-install-recommends gdb
WORKDIR /usr/src/gomoku-ai
COPY . .
RUN gcc src/*.c -std=c99 -Wall -Wextra -g -o g

#FROM node:14
#WORKDIR /usr/src/gomoku-ai/gomoku-server
#COPY --from=gcc /usr/src/gomoku-ai ./..
#RUN npm install
#EXPOSE 7682
#CMD ["npm", "start"]