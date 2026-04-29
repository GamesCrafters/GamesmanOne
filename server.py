from flask import Flask
import psutil
import time
from datetime import datetime, timezone
import subprocess
import shlex

app = Flask(__name__)

start_time = time.time()

def format_time(seconds: float) -> str:
    seconds = int(seconds)
    return f"{seconds // 86400}d {(seconds % 86400) // 3600}h {(seconds % 3600) // 60}m {seconds % 60}s"

def build_error_response(message):
    return "{ \"error\": \"" + message + "\" }"

def run_command(command):
    try:
        result = subprocess.run(shlex.split(command), check=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        return build_error_response(e.stderr)

@app.route("/<game_name>/<variant_id>/")
def getstart(game_name, variant_id):
    return run_command(f"bin/gamesman getstart {game_name} {variant_id}")

@app.route("/<game_name>/<variant_id>/<position>")
def query(game_name, variant_id, position):
    return run_command(f"bin/gamesman query -- {game_name} {variant_id} {position}")

@app.route('/health')
def get_health():
    current_process = psutil.Process()
    with current_process.oneshot():
        return {
            'status': 'ok',
            'http_code': 200,
            'uptime': format_time(time.time() - start_time),
            'cpu_usage': f"{current_process.cpu_percent():.2f}%",
            'memory_usage': f"{current_process.memory_percent():.2f}%",
            'timestamp': datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace('+00:00', 'Z')
        }, 200

if __name__ == "__main__":
    from waitress import serve
    serve(app, host="0.0.0.0", port=8084)
