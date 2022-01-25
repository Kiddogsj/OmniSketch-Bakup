from sre_constants import SUCCESS
from flask import Flask, request, render_template
import subprocess

app = Flask(__name__)

@app.route("/")
def dashboard():
    return render_template('Home.html')

@app.route("/parser")
def parser():
    return render_template('Parser.html')

@app.route("/benchmark")
def benchmark():
    return render_template('Benchmark.html')

@app.route('/benchmarkok', methods=['GET', 'POST'])
def benchmark_response():
    with open('../test/config.ini', 'w') as fd:
        fd.write('[Common]\n\n')
        fd.write('['+request.form['sketch_name']+']\n\n')
        params = request.form['parameters']
        params = params.replace(';', '\n')
        params = params.replace('=', '@')
        params = params.replace('@', ' = ')
        fd.write(params)
    cmd = 'ctest -R ' + request.form['test_name'] + ' -VV'
    run = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True, cwd='../build')
    with open('result.log', 'w') as fd:
        for out in run.stdout.readlines():
            fd.write(str(out, encoding='utf-8'))
    return render_template('BenchmarkOK.html')

@app.route("/vis")
def vis():
    return render_template('Vis.html')

@app.route('/parserok', methods=['GET', 'POST'])
def parser_response():
    with open('parser.conf', 'w') as fd:
        fd.write("[parser]\n")
        fd.write('packet_count = ' + request.form['packet_count']+"\n")
        fd.write('flow_count = ' + request.form['flow_count']+"\n")
        fd.write('epoch_num = ' + request.form['epoch_num']+"\n")
        fd.write('epoch_len = ' + request.form['epoch_len']+"\n")
        fd.write('write_to_binary_file = ' + request.form['write_to_binary_file']+"\n")
        fd.write('write_to_txt_file = ' + request.form['write_to_txt_file']+"\n")
        fd.write('write_to_pcap_file = ' + request.form['write_to_pcap_file']+"\n")
        fd.write('network_endian = ' + request.form['network_endian']+"\n")
        fd.write('key_len = ' + request.form['key_len']+"\n")
        fd.write('val_timestamp = ' + request.form['val_timestamp']+"\n")
        fd.write('val_length = ' + request.form['val_length']+"\n\n")
        fd.write('input_path = ' + request.form['input_path']+"\n")
        fd.write('output_path = ' + request.form['output_path']+"\n")
    if request.form['run'] == 'true':
        cmd = '../bin/parser -c parser.conf'
        run = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
        for out in run.stdout.readlines():
            print(out)
    return render_template('ParserOK.html')
