FROM golang:1.23-alpine AS build
WORKDIR /app
COPY . .
RUN go build -o server main.go

FROM alpine:latest
WORKDIR /app
COPY --from=build /app/server .
EXPOSE 3000
CMD [ "./server" ]
