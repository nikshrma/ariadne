package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
)

func loggerMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// fmt.Printf("Received %s request to %s\n", r.Method, r.URL.Path)
		next.ServeHTTP(w, r)
	})
}

func main() {
	mux := http.NewServeMux()

	mux.HandleFunc("GET /{$}", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("Hello from GET!"))
	})

	mux.HandleFunc("POST /submit", func(w http.ResponseWriter, r *http.Request) {
		body, _ := io.ReadAll(r.Body)
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("Received POST data: " + string(body)))
	})

	mux.HandleFunc("GET /api/user", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(`{"name": "Ariadne", "role": "admin"}`))
	})

	mux.HandleFunc("GET /error", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusInternalServerError)
		w.Write([]byte("Internal server error"))
	})

	handler := loggerMiddleware(mux)

	fmt.Println("Go net/http benchmark server running on port 3000")
	log.Fatal(http.ListenAndServe(":3000", handler))
}
