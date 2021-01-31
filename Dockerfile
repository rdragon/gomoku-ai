FROM gcc:4.9 AS gcc
WORKDIR /usr/src/gomoku-ai
COPY . .
RUN mkdir -p bin/release && gcc src/*.c -std=c99 -Wall -Wextra -O2 -o bin/release/gomoku-ai

FROM node:14
WORKDIR /usr/src/gomoku-ai/gomoku-server
COPY --from=gcc /usr/src/gomoku-ai ./..
RUN npm install
EXPOSE 7682
CMD ["npm", "start"]