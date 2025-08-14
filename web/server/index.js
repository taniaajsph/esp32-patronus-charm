const express = require("express");
const app = express();
const path = require("path");
const port = 5000;

// Define the public directory path
 const publicDir = path.join(__dirname, "../public");
app.use(express.static(publicDir));

console.log(`📂 Serving static files from: ${publicDir}`);

// Add CORS support
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

// Use Map for better client management
const clients = new Map();

app.get("/events", (req, res) => {
  res.setHeader("Content-Type", "text/event-stream");
  res.setHeader("Cache-Control", "no-cache");
  res.setHeader("Connection", "keep-alive");
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.flushHeaders();

  const clientId = Date.now();
  clients.set(clientId, res);
  console.log(`🧙 Client ${clientId} connected`);

  // Send initial message
  res.write(`id: ${clientId}\n`);
  res.write("event: connected\n");
  res.write("data: Welcome to Patronus Magic\n\n");

  // Heartbeat every 10 seconds
  const heartbeat = setInterval(() => {
    try {
      res.write(":heartbeat\n\n");
    } catch (err) {
      console.error(`💔 Heartbeat failed for client ${clientId}:`, err);
      clearInterval(heartbeat);
    }
  }, 10000);

  req.on("close", () => {
    console.log(`🚫 Client ${clientId} disconnected`);
    clearInterval(heartbeat);
    clients.delete(clientId);
  });
});

app.post("/cast", (req, res) => {
  console.log("✨ Wand gesture detected!");
  console.log(`🌐 Request from: ${req.ip}`);

  console.log(`📤 Broadcasting to ${clients.size} clients`);

  // Send to all connected clients
  let sendCount = 0;
  clients.forEach((clientRes, clientId) => {
    try {
      clientRes.write("event: message\n");
      clientRes.write("data: trigger\n\n");
      sendCount++;
    } catch (err) {
      console.error(`❌ Failed to send to client ${clientId}:`, err);
      clients.delete(clientId);
    }
  });

  console.log(`✅ Trigger sent to ${sendCount} clients`);
  res.status(200).send("Spell cast!");
});

app.listen(port, () => {
  console.log(`⚡ Server running at http://localhost:${port}`);
});