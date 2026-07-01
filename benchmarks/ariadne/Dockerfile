FROM gcc:12 AS build
WORKDIR /app
COPY . .
RUN g++ -std=c++20 -pthread benchmarks/ariadne/main.cpp src/server.cpp src/response.cpp src/threadpool.cpp -o ariadne

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /app/ariadne .
EXPOSE 3000
CMD [ "./ariadne" ]
