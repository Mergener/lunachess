import os
import subprocess
import time
import json
from time import gmtime, strftime

# Load settings
with open('sprt-config.json', 'r') as f:
    settings = json.load(f)

cutechess_path  = settings.get('cutechessPath') or 'cutechess-cli'
tc              = settings.get('timeControl') or '8+0.08'
interval        = settings.get('ratingDisplayInterval') or 10
threads         = settings.get('concurrency') or 1
games           = settings.get('games')
filter          = settings.get('filter') or True
debug           = settings.get('debug') or False
regression      = settings.get('regression') or False
engines         = settings.get('engines') or []
openings_path   = settings.get('openings')
game_out_folder = settings.get('gameOutputFolder')
sprt_alpha      = settings.get('sprtAlpha') or 0.05
sprt_beta       = settings.get('sprtBeta') or 0.05
elo0            = settings.get('elo0') or 0
elo1            = settings.get('elo1') or 10

if games != None and regression:
    print('SPRT Regression test requested -- specified number of games will be ignored.')

improvement = not regression and games == None
if improvement:
    print('Testing mode is set to SPRT - improvement.')
elif regression:
    print('Testing mode is set to SPRT - regression.')
else:
    print(f'Testing mode is set to number of games ({games})')

if openings_path:
    openings_path = os.path.abspath(openings_path)

# Prepare command
command = cutechess_path

if len(engines) < 2:
    raise Exception('Expected at least two engines to compete.')

for engine in engines:
    cmd = engine.get('cmd')
    if cmd == None:
        raise Exception('Expected an engine command ("cmd").')

    name = engine.get('name')
    if name == None:
        # Use command file name as fallback
        name = cmd.split('/')[-1]

    protocol = engine.get('protocol')
    if protocol == None:
        raise Exception('Expected an engine protocol ("protocol") (ex: uci)')
    
    timemargin = engine.get('timemargin') or 0

    command += f' -engine name=\"{name}\" proto={protocol} timemargin={timemargin} cmd=\"{cmd}\"'


if openings_path:
    opening_format = 'pgn' if openings_path.endswith('.pgn') else 'epd'
    command += f' -openings file={openings_path} order=random format={opening_format}'

command += f' -concurrency {threads}'
command += f' -ratinginterval {interval}'
if games:
    command += f' -games {games}'

if game_out_folder:
    if not os.path.exists(game_out_folder):
        os.makedirs(game_out_folder)

    name = strftime('%Y-%m-%d_%H-%M-%S', gmtime())
    command += f' -pgnout {os.path.join(game_out_folder, f"{name}.pgn")}'

command += ' -repeat'
if debug:
    command += ' -debug'

is_sprt = improvement or regression
if is_sprt:
    command += f' -sprt elo0={elo0} elo1={elo1} alpha={sprt_alpha} beta={sprt_beta}'

command += f' -each tc={tc}'
command += f' option.Hash=8'

# Finally, execute the command

t0 = None
games_finished = 0

p = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)

try:
    line_begin = 'SPRT: ' if is_sprt else 'Elo difference: '
    while p.poll() is None:
        line = p.stdout.readline().decode('utf-8') 
        line = line.rstrip('\r\n')

        if filter:
            if line.startswith('Warning: Illegal PV move'):
                continue
            elif line.startswith('Warning: PV:'):
                continue
            elif line.startswith('Started game'):
                if t0 == None:
                    t0 = time.time()
                continue
            elif line.startswith('Finished game'):
                continue
            elif line.startswith('...'):
                continue

        if line.startswith('Score of'):
            parts = line.split(' ')
            if len(parts) == 13:
                games_finished = int(parts[12])
        elif line.startswith('Rank'):
            print('')

        show = games_finished <= interval or games_finished % interval == 0 or games_finished == games

        if show:
            print(line)

        if show and filter and line.startswith(line_begin) and games_finished > 0:
            if not is_sprt:
                time_taken = time.time() - t0
                time_per_game = time_taken / games_finished
                games_left = games - games_finished
                time_left = int(time_per_game * games_left)
                hours_left = time_left // 3600
                minutes_left = (time_left % 3600) // 60
                seconds_left = time_left % 60
                print(f'Time left:  {hours_left:02d}h {minutes_left:02d}m {seconds_left:02d}s')
                print(f'Games/min:  {60/time_per_game:.2f}')
            print('')
except:
    pass

if t0:
    t1 = time.time()
    time_taken = t1 - t0
    print(f'Time taken: {time_taken:.0f}s')
    if games_finished > 0:
        print(f'Game time:  {time_taken/games_finished*threads:.2f}s')

print(f'Games:      {games_finished}')