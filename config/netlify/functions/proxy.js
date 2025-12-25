// Netlify Function: simple proxy with allowed-hosts check
// Expects query param `url` containing the upstream URL to fetch.
// Returns upstream response as text with content-type preserved.

const ALLOWED_HOSTS = new Set([
  'api.open-meteo.com',
  'api.familywall.com',
  'rss.tagesschau.de',
  'www.tagesschau.de',
]);

export const handler = async function(event) {
  try{
    const qs = event.queryStringParameters || {};
    const url = qs.url;
    if(!url) return { statusCode: 400, body: 'missing url' };
    let parsed;
    try{ parsed = new URL(url); }catch(e){ return { statusCode: 400, body: 'invalid url' }; }
    if(!ALLOWED_HOSTS.has(parsed.hostname)) return { statusCode: 403, body: 'host not allowed' };

    // Use global fetch (Node 18+). Some upstreams reject requests without a
    // browser-like User-Agent header, so include common headers. For binary
    // responses we'd need to base64-encode, but our client only needs text
    // (ics, json, xml, html).
    const fetchOpts = {
      headers: {
        'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120 Safari/537.36',
        'accept': 'application/rss+xml, application/xml, text/xml, text/html, */*;q=0.9'
      }
    };
    const res = await fetch(parsed.href, fetchOpts);
    const contentType = res.headers.get('content-type') || 'text/plain; charset=utf-8';
    const body = await res.text();
    return {
      statusCode: res.status,
      headers: { 
        'content-type': contentType,
        'access-control-allow-origin': '*',
        'access-control-allow-methods': 'GET, OPTIONS',
        'access-control-allow-headers': 'Content-Type'
      },
      body
    };
  }catch(err){
    return { statusCode: 500, body: String(err) };
  }
};
