// server.js
// Node 18+ recommended
import express from 'express';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const app = express();
const PORT = process.env.PORT || 3000;

// (No server-side camera broadcasting — clients use their local cameras)

// Allowed hosts to avoid being an open proxy
const ALLOWED = [
  'api.familywall.com',
  'www.tagesschau.de',
  'tagesschau.de'
];

// allow local ESP32 IP for proxying
ALLOWED.push('192.168.178.50');

// Serve static files (HTML, CSS, JS)
app.use(express.static(join(__dirname, '..')));

app.use((req, res, next) => {
  // Allow CORS for the dashboard
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET,OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
  if (req.method === 'OPTIONS') return res.sendStatus(200);
  next();
});

// Simple /proxy?url=ENCODED_URL
app.get('/proxy', async (req, res) => {
  try {
    const url = req.query.url;
    if (!url) return res.status(400).send('missing url');
    const decoded = decodeURIComponent(url);
    let host;
    try {
      host = new URL(decoded).host;
    } catch (err) {
      return res.status(400).send('invalid url');
    }
    // ensure allowed
    if (!ALLOWED.some(a => host.includes(a))) {
      return res.status(403).send('host not allowed');
    }
    const upstream = await fetch(decoded);
    const contentType = upstream.headers.get('content-type') || 'application/octet-stream';
    res.setHeader('Content-Type', contentType);
    const body = await upstream.arrayBuffer();
    res.status(upstream.status).send(Buffer.from(body));
  } catch (e) {
    console.error('proxy error', e);
    res.status(500).send('proxy error');
  }
});

// Serve host camera as a single MJPEG stream for all clients
// no /camera endpoint

// serve a tiny health route
app.get('/', (req, res) => res.send('Proxy running. Use /proxy?url=<encoded-url>') );

app.listen(PORT, () => {
  console.log(`Proxy listening on http://localhost:${PORT}`);
});
