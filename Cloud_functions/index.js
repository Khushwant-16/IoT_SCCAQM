const functions = require("firebase-functions");
const admin = require("firebase-admin");
const cors = require("cors")({origin: true});

admin.initializeApp();
const db = admin.firestore();

exports.sccaqmDataReceiver = functions.https.onRequest((request, response) => {
  cors(request, response, async () => {
    
    // Handle GET requests (from ESP32 asking for the mode)
    if (request.method === "GET") {
      try {
        const configRef = db.collection("config").doc("system");
        const docSnap = await configRef.get();
        
        // --- THIS IS THE FIX ---
        // Changed docSnap.exists() to docSnap.exists
        if (docSnap.exists) { 
          // Send back the config data (e.g., {"mode": "Away"})
          response.status(200).send(docSnap.data());
        } else {
          // If it doesn't exist, send a safe default
          response.status(200).send({ mode: "Home" });
        }
      } catch (error) {
        functions.logger.error("Error fetching config:", error);
        response.status(500).send({ error: "Failed to fetch config" });
      }
      return; // Stop here for GET requests
    }

    // Handle POST requests (from ESP32 sending sensor data)
    if (request.method === "POST") {
      try {
        const dataPayload = request.body;
        
        if (typeof dataPayload !== 'object' || dataPayload === null) {
          throw new Error("Invalid JSON payload.");
        }

        const finalPayload = {
          ...dataPayload,
          timestamp: admin.firestore.FieldValue.serverTimestamp(),
        };

        const batch = db.batch();
        const statusRef = db.collection("status").doc("cabin_latest");
        batch.set(statusRef, finalPayload);
        const logRef = db.collection("logs").doc();
        batch.set(logRef, finalPayload);

        await batch.commit();
        response.status(200).send({status: "success", received: dataPayload});

      } catch (error) {
        functions.logger.error("Error processing request:", error);
        response.status(500).send({status: "error", message: error.message});
      }
      return; // Stop here for POST requests
    }
    
    // Handle any other request types
    response.status(400).send("Invalid request method.");
  });
});