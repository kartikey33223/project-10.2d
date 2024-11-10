import paho.mqtt.client as mqtt
import tkinter as tk
from threading import Thread
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import pandas as pd

# Initialize lists to store temperature and pulse data
temperature_data = []
pulse_data = []

# Initialize lists to store temperature and pulse data into excel
temperature_data_excel = []
pulse_data_excel = []

count = 0

# Function to be executed when a message is received
def on_message(client, userdata, message):
    try:
        # Decoding the message payload
        payload = message.payload.decode("utf-8")
        print(f"Received message: {payload} from topic: {message.topic}")

        # Perform some operations on the received message
        processed_data = process_message(payload)
        display_data(processed_data)
    except Exception as e:
        print(f"Error processing message: {e}")


# Function to process the received message
def process_message(payload):
    global count
    # Example processing: Let's assume the message contains temperature and pulse
    try:
        temperature = float(payload.split(",")[0])
        pulse = float(payload.split(",")[1])

        # Append data to lists
        temperature_data.append(temperature)
        pulse_data.append(pulse)

        if count > 10:
            # Append data to Excel lists
            temperature_data_excel.append(temperature)
            pulse_data_excel.append(pulse)
            count = 0
        else:
            count += 1

        return f"Temperature: {temperature} °F, Pulse: {pulse} BPM"
    except ValueError:
        return f"Invalid data: {payload}"


# Function to be executed when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        # Subscribing to a topic once connected
        client.subscribe("SB_PI_DATA")
    else:
        print(f"Failed to connect, return code {rc}")


# Function to start the MQTT loop in a separate thread
def start_mqtt():
    client.loop_forever()


# Function to display data in the GUI
def display_data(data):
    # Update the GUI with the processed data
    output_var.set(data)
    plot_data()


def plot_data():
    # Clear previous plots
    ax1.clear()
    ax2.clear()

    # Create subplots for temperature and pulse
    ax1.plot(temperature_data, label='Temperature (°F)', color='red')
    ax1.set_title('Temperature over Time')
    ax1.set_xlabel('Data Points')
    ax1.set_ylabel('Temperature (°F)')
    ax1.legend()

    ax2.plot(pulse_data, label='Pulse (BPM)', color='blue')
    ax2.set_title('Pulse over Time')
    ax2.set_xlabel('Data Points')
    ax2.set_ylabel('Pulse (BPM)')
    ax2.legend()

    # Adjust spacing between subplots
    fig.subplots_adjust(hspace=0.4)  # Adjust the height space between subplots

    # Draw the canvas
    canvas.draw()


def on_save_btn_click():
    # Create a DataFrame to save to Excel
    df = pd.DataFrame({
        'Temperature (°F)': temperature_data_excel,
        'Pulse (BPM)': pulse_data_excel
    })

    # Save the DataFrame to an Excel file
    df.to_excel('mqtt_data.xlsx', index=False)
    print("Data saved to mqtt_data.xlsx")


# MQTT Client Setup
client = mqtt.Client()

# Assign the on_connect and on_message functions
client.on_connect = on_connect
client.on_message = on_message

# Set MQTT broker address and port (you can replace this with your own broker)
broker_address = "broker.emqx.io"  # Example public broker
broker_port = 1883  # Default port for MQTT

# Connect to the broker
client.connect(broker_address, broker_port, keepalive=60)

# Start the MQTT client loop in a separate thread
mqtt_thread = Thread(target=start_mqtt, daemon=True)
mqtt_thread.start()

# GUI Setup
root = tk.Tk()
root.geometry("900x900")
root.resizable(False, False)
root.title("SMART BAND MONITOR")

heading_label = tk.Label(root, text="SMART BAND MONITOR", font=("Times New Roman", 30, "bold"))
heading_label.pack(pady=20)

# Create a figure for the plots
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(6, 6))
ax1.set_title('Temperature over Time')
ax2.set_title('Pulse over Time')

# Adjust spacing between subplots
fig.subplots_adjust(hspace=0.4)  # Adjust the height space between subplots

# Create a canvas to embed the plot in the Tkinter GUI
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(pady=20)

# Variable to hold output data
output_var = tk.StringVar()
output_var.set("Temperature: 28 °F, Pulse: 141 BPM")
output_label = tk.Label(root, textvariable=output_var, font=("Arial", 18, "bold"), bg="black", fg="white", padx=10)
output_label.pack(pady=20)

# Button to save the Data
save_btn = tk.Button(root, text="Save Data", command=on_save_btn_click, bg="green", fg="white",
                     font=("Arial", 18, "bold"))
save_btn.pack(pady=20)

# Start the GUI event loop
root.mainloop()
