import os


# with open('~code.txt', 'r') as fi:
#     code = fi.readlines()
#     lines = ['"'+l.strip().replace('"', '\\"')+'"' for l in code]
#     with open('~snippet.txt', 'w') as f:
#         for l in lines:
#             f.write(l)
#             f.write(',\n')

with open('~code.txt', 'r') as fi:
    with open('~snippet.txt', 'w') as f:
        for l in fi:
            f.write('"')
            f.write(l.strip().replace('"', '\\"'))
            f.write('"')
            f.write(',\n')
