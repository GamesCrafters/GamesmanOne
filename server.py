from flask import Flask
import subprocess
import shlex

app = Flask(__name__)

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

if __name__ == "__main__":
    from waitress import serve
    serve(app, host="0.0.0.0", port=8084)
