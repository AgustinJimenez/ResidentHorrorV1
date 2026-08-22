import * as net from "net";

const UE_HOST = process.env.UE_HOST || "127.0.0.1";
const UE_PORT = parseInt(process.env.UE_PORT || "9877", 10);

export async function sendToUnreal(command, params = {}) {
  return new Promise((resolve, reject) => {
    const client = new net.Socket();
    let data = "";

    client.setNoDelay(true);
    client.setTimeout(30000);

    client.connect(UE_PORT, UE_HOST, () => {
      const request = JSON.stringify({ command, params });
      client.write(request);
    });

    client.on("data", (chunk) => {
      data += chunk.toString("utf8");
      if (data.endsWith("\n")) {
        const trimmed = data.trim();
        try {
          const parsed = JSON.parse(trimmed);
          client.destroy();
          console.error(`Received ${trimmed.length} bytes from Unreal Engine`);
          resolve(parsed);
        } catch {
          // Response not complete yet (newline inside string or payload truncated across chunks), keep accumulating
        }
      }
    });

    client.on("timeout", () => {
      client.destroy();
      reject(new Error("Connection timeout"));
    });

    client.on("error", (err) => {
      reject(new Error(`Connection error: ${err.message}`));
    });

    client.on("close", () => {
      if (data && !data.includes("\n")) {
        try {
          resolve(JSON.parse(data.trim()));
        } catch {
          // Ignore parse errors on close; handled above when data is complete.
        }
      }
    });
  });
}
