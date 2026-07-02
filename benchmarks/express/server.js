const express = require("express");
const app = express();

function fib(n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}
app.use(express.text());
app.use((req, res, next) => {
  // console.log(`Received ${req.method} request to ${req.path}`);
  next();
});
app.get("/api/compute", (req, res) => {
  const result = fib(30);
  res.json({ result });
});

app.get("/", (req, res) => {
  res.status(200).send("Hello from GET!");
});

app.post("/submit", (req, res) => {
  res.status(200).send("Received POST data: " + req.body);
});

app.get("/api/user", (req, res) => {
  res.status(200).json({ name: "Ariadne", role: "admin" });
});

app.get("/error", (req, res) => {
  res.status(500).send("Internal server error");
});

app.listen(3000, () => {
  console.log("Express benchmark server running on port 3000");
});
