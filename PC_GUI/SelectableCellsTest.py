import tkinter as tk
from tkinter import ttk

# Function to handle cell selection
def handle_cell_selection(event):
  print("In handle_cell_selection")
#   item = tree.identify_element(event.x, event.y)
  item = tree.identify_region(event.x, event.y)
  col = tree.identify_column(event.x)
  row = tree.identify_row(event.y)
  print(item)
  if item == "tree":
    print(f"Cell selected - Column: {col},")
    print(f"Row: {row}")

# Create the main window
root = tk.Tk()
root.title("Selectable Treeview Table")

# Create the Treeview widget
tree = ttk.Treeview(root, columns=("Name", "Age", "City"))

# Define column headings
tree.heading("#0", text="ID")
tree.heading("Name", text="Name")
tree.heading("Age", text="Age")
tree.heading("City", text="City")

# Set column widths (optional)
tree.column("#0", width=50)
tree.column("Name", width=150)
tree.column("Age", width=50)
tree.column("City", width=100)

# Insert sample data
data = [("Alice", 30, "New York"), ("Bob", 25, "London"), ("Charlie", 42, "Paris")]
for i, item in enumerate(data):
  tree.insert("", tk.END, iid=i, values=item)

# Bind selection event to the treeview
tree.bind("<ButtonRelease-1>", handle_cell_selection)

# Display the treeview
tree.grid(padx=10, pady=10)

# Run the main event loop
root.mainloop()