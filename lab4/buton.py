from gpiozero import LED, Button
from signal import pause

led = LED(17)
buton = Button(27, bounce_time=0.05)

def degistir():
    led.toggle()
    print("LED:", "yanik" if led.is_lit else "sonuk")

buton.when_pressed = degistir
print("Butona bas (cikmak icin Ctrl+C)")
pause()
