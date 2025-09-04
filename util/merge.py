import os

HC_DIR = os.path.join(os.path.dirname(__file__), '../voices')
VOICES = ['melody', 'bass2', 'bass1', 'bass0']
MERGED_PATH = os.path.join(HC_DIR, 'boc-olson.txt')
TAPE_END = '\n@'

voice_contents = []
for voice in VOICES:
    with open(os.path.join(HC_DIR, f'{voice}.txt'), 'r', encoding='utf-8') as f:
        voice_contents.append(f.read())

merged = TAPE_END.join(voice_contents) + TAPE_END

with open(MERGED_PATH, 'w', encoding='utf-8') as f:
    f.write(merged)