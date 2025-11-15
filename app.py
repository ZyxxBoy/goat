import os
from flask import Flask, render_template, jsonify
from dotenv import load_dotenv

load_dotenv()  # baca file .env

app = Flask(__name__)
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', 'dev-key')

# ----- Contoh data dummy dari "backend" -----
GOATS = [
    {"id": "001", "weight": 45, "gender": "Male"},
    {"id": "002", "weight": 38, "gender": "Female"},
    {"id": "003", "weight": 52, "gender": "Male"},
]

@app.route("/")
def dashboard():
    # kirim data kambing ke template (bisa nanti ganti dari DB / API IoT)
    return render_template("index.html", goats=GOATS)

@app.route("/wifi-manager")
def wifi_manager():
    return render_template("wifi-manager.html")

# contoh endpoint API kalau mau dipanggil dari JS
@app.route("/api/goats")
def api_goats():
    return jsonify(GOATS)

if __name__ == "__main__":
    app.run(debug=True)
