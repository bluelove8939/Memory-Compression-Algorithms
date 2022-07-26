import math

def progressbar(status, total, scale=20):
    cnt = math.ceil(status / total * scale)
    return f"[{''.join(['='] * cnt)}{''.join([' '] * (scale - cnt))}]"