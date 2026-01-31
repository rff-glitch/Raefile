from PIL import Image, ImageDraw

def create_arrow_up(filename):
    img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Draw arrow up (white, bold)
    # Triangle top
    draw.polygon([(32, 8), (8, 32), (56, 32)], fill='white')
    # Rect bottom
    draw.rectangle([(24, 32), (40, 56)], fill='white')
    img.save(filename)

def create_refresh(filename):
    img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Draw simple circular arrow (arc + arrow head)
    # Arc
    bbox = (10, 10, 54, 54)
    draw.arc(bbox, start=30, end=330, fill='white', width=6)
    # Arrow head at end (approx 330 deg)
    # Just simplistic triangle at (48, 20) approx
    draw.polygon([(48, 10), (60, 22), (48, 30)], fill='white')
    img.save(filename)

if __name__ == "__main__":
    create_arrow_up("assets/ic_arrow_up.png")
    create_refresh("assets/ic_refresh.png")
