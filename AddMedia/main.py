import tkinter as tk
from tkinter import ttk
import json
from pathlib import Path
import os
from tkinter import messagebox

# Import existing functionality
from cook import process_media_directory
from add_to_db import process_data
from chunkonize import chunkonize


class MediaProcessingGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Media Processing Interface")
        self.root.geometry("800x900")  # Made taller to accommodate new fields

        # Default values
        self.MEDIA_DIR = './media'
        self.COVERS_DIR = "G://GhostCovers"
        self.CHUNKS_DIR = "G://GhostChunks"
        self.API_KEY = "3dec1c49"
        self.DATABASE_PATH = "G://ghost.db"

        # Create main container
        self.main_container = ttk.Frame(self.root, padding="10")
        self.main_container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Create paths configuration
        self.create_paths_config()

        # Create two main sections
        self.create_file_tree()
        self.create_config_panel()

        # Progress bar and status
        self.status_var = tk.StringVar(value="Ready")
        self.status_label = ttk.Label(self.main_container, textvariable=self.status_var)
        self.status_label.grid(row=4, column=0, columnspan=2, pady=5)

        self.progress = ttk.Progressbar(self.main_container, length=300, mode='determinate')
        self.progress.grid(row=5, column=0, columnspan=2, pady=5, sticky=(tk.W, tk.E))

        # Process button
        self.process_btn = ttk.Button(self.main_container, text="Process Media", command=self.process_media)
        self.process_btn.grid(row=6, column=0, columnspan=2, pady=5)

        # Initialize
        self.load_media_directory()

    def create_paths_config(self):
        """Create configuration fields for paths and API key"""
        config_frame = ttk.LabelFrame(self.main_container, text="Configuration", padding="5")
        config_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

        # Media Directory
        ttk.Label(config_frame, text="Media Directory:").grid(row=0, column=0, sticky=tk.W, padx=5)
        self.media_dir_var = tk.StringVar(value=self.MEDIA_DIR)
        self.media_dir_entry = ttk.Entry(config_frame, textvariable=self.media_dir_var, width=40)
        self.media_dir_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=5)

        # Covers Directory
        ttk.Label(config_frame, text="Covers Directory:").grid(row=1, column=0, sticky=tk.W, padx=5)
        self.covers_dir_var = tk.StringVar(value=self.COVERS_DIR)
        self.covers_dir_entry = ttk.Entry(config_frame, textvariable=self.covers_dir_var, width=40)
        self.covers_dir_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=5)

        # Chunks Directory
        ttk.Label(config_frame, text="Chunks Directory:").grid(row=2, column=0, sticky=tk.W, padx=5)
        self.chunks_dir_var = tk.StringVar(value=self.CHUNKS_DIR)
        self.chunks_dir_entry = ttk.Entry(config_frame, textvariable=self.chunks_dir_var, width=40)
        self.chunks_dir_entry.grid(row=2, column=1, sticky=(tk.W, tk.E), padx=5)

        # API Key
        ttk.Label(config_frame, text="OMDb API Key:").grid(row=3, column=0, sticky=tk.W, padx=5)
        self.api_key_var = tk.StringVar(value=self.API_KEY)
        self.api_key_entry = ttk.Entry(config_frame, textvariable=self.api_key_var, width=40)
        self.api_key_entry.grid(row=3, column=1, sticky=(tk.W, tk.E), padx=5)

        # API Key
        ttk.Label(config_frame, text="Database path:").grid(row=4, column=0, sticky=tk.W, padx=5)
        self.database_var = tk.StringVar(value=self.DATABASE_PATH)
        self.database_entry = ttk.Entry(config_frame, textvariable=self.database_var, width=40)
        self.database_entry.grid(row=4, column=1, sticky=(tk.W, tk.E), padx=5)

        # Refresh button
        ttk.Button(config_frame, text="Refresh Media", command=self.refresh_media).grid(row=5, column=0, columnspan=2,
                                                                                        pady=10)

    def create_file_tree(self):
        """Create the file tree view panel"""
        frame = ttk.LabelFrame(self.main_container, text="Media Files", padding="5")
        frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)

        # Create treeview
        self.tree = ttk.Treeview(frame, selectmode='browse')
        self.tree.heading('#0', text='Media Files', anchor=tk.W)
        self.tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Scrollbar
        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=self.tree.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.tree.configure(yscrollcommand=scrollbar.set)

        # Bind selection event
        self.tree.bind('<<TreeviewSelect>>', self.on_select)

    def create_config_panel(self):
        """Create the configuration panel"""
        self.config_frame = ttk.LabelFrame(self.main_container, text="Media Type", padding="5")
        self.config_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)

        # Media type selection
        self.media_type_var = tk.StringVar(value="standalone")

        ttk.Radiobutton(self.config_frame, text="Standalone Movie",
                        variable=self.media_type_var, value="standalone").grid(row=0, column=0, sticky=tk.W)
        ttk.Radiobutton(self.config_frame, text="Movie Collection",
                        variable=self.media_type_var, value="collection").grid(row=1, column=0, sticky=tk.W)
        ttk.Radiobutton(self.config_frame, text="TV Series",
                        variable=self.media_type_var, value="series").grid(row=2, column=0, sticky=tk.W)

        # Create collection details frame
        self.collection_frame = ttk.LabelFrame(self.config_frame, text="Collection Details", padding="5")
        self.collection_frame.grid(row=3, column=0, sticky=(tk.W, tk.E), pady=5)
        self.collection_frame.grid_remove()  # Hidden by default

        # Collection ID
        ttk.Label(self.collection_frame, text="Collection ID:").grid(row=0, column=0, sticky=tk.W)
        self.collection_id_entry = ttk.Entry(self.collection_frame, width=40)
        self.collection_id_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=5)

        # Collection Title
        ttk.Label(self.collection_frame, text="Collection Title:").grid(row=1, column=0, sticky=tk.W)
        self.collection_title_entry = ttk.Entry(self.collection_frame, width=40)
        self.collection_title_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=5)

        # Collection Description
        ttk.Label(self.collection_frame, text="Description:").grid(row=2, column=0, sticky=tk.W)
        self.collection_description_text = tk.Text(self.collection_frame, width=40, height=4)
        self.collection_description_text.grid(row=2, column=1, sticky=(tk.W, tk.E), padx=5)

        # URL configuration
        self.url_frame = ttk.LabelFrame(self.config_frame, text="IMDb URLs", padding="5")
        self.url_frame.grid(row=4, column=0, sticky=(tk.W, tk.E), pady=5)

        # Main URL entry (for series only)
        self.main_url_label = ttk.Label(self.url_frame, text="IMDb URL:")
        self.main_url_label.grid(row=0, column=0, sticky=tk.W)
        self.main_url_entry = ttk.Entry(self.url_frame, width=40)
        self.main_url_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=5)

        # Individual episodes checkbox for TV series
        self.individual_episodes_var = tk.BooleanVar(value=False)
        self.episode_urls_check = ttk.Checkbutton(
            self.url_frame,
            text="Specify individual episode URLs",
            variable=self.individual_episodes_var,
            command=self.toggle_episode_urls
        )

        # Container for individual file URLs
        self.file_urls_frame = ttk.Frame(self.url_frame)
        self.file_urls_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E))
        self.file_url_entries = {}

    def refresh_media(self):
        """Update directories and refresh media list"""
        self.MEDIA_DIR = self.media_dir_var.get()
        self.COVERS_DIR = self.covers_dir_var.get()
        self.CHUNKS_DIR = self.chunks_dir_var.get()
        self.API_KEY = self.api_key_var.get()

        os.makedirs(self.MEDIA_DIR, exist_ok=True)
        os.makedirs(self.COVERS_DIR, exist_ok=True)
        os.makedirs(self.CHUNKS_DIR, exist_ok=True)

        self.load_media_directory()

    def on_media_type_change(self, *args):
        """Handle media type changes"""
        media_type = self.media_type_var.get()

        # Hide all frames first
        self.collection_frame.grid_remove()
        self.episode_urls_check.grid_remove()
        self.main_url_label.grid()
        self.main_url_entry.grid()

        if media_type == 'series':
            self.episode_urls_check.grid(row=1, column=0, columnspan=2, sticky=tk.W)
            if self.individual_episodes_var.get():
                self.show_file_urls()
        elif media_type == 'collection':
            self.collection_frame.grid()
            self.main_url_label.grid_remove()
            self.main_url_entry.grid_remove()
            self.show_file_urls()
        else:  # standalone
            self.hide_file_urls()

    def load_media_directory(self):
        """Load the media directory structure into the tree"""
        media_path = Path(self.media_dir_var.get())
        if not media_path.exists():
            messagebox.showerror("Error", f"Media directory {self.media_dir_var.get()} not found!")
            return

        # Clear existing items
        for item in self.tree.get_children():
            self.tree.delete(item)

        # Add files in root
        for file in media_path.glob('*'):
            if file.is_file() and file.suffix.lower() in ['.mp4', '.mkv', '.avi']:
                self.tree.insert('', 'end', text=file.name, values=(str(file),))

        # Add folders
        for folder in media_path.glob('*'):
            if folder.is_dir():
                folder_item = self.tree.insert('', 'end', text=folder.name, values=(str(folder),))
                # Add files in folder
                for file in folder.glob('*'):
                    if file.suffix.lower() in ['.mp4', '.mkv', '.avi']:
                        self.tree.insert(folder_item, 'end', text=file.name, values=(str(file),))

    def toggle_episode_urls(self):
        """Toggle visibility of episode URL entries"""
        if self.individual_episodes_var.get():
            self.show_file_urls()
        else:
            self.hide_file_urls()

    def show_file_urls(self):
        """Show URL entries for individual files"""
        for widget in self.file_urls_frame.winfo_children():
            widget.destroy()

        selected_item = self.tree.selection()[0]
        path = Path(self.tree.item(selected_item)['values'][0])

        if path.is_dir():
            row = 0
            for file in path.glob('*'):
                if file.suffix.lower() in ['.mp4', '.mkv', '.avi']:
                    ttk.Label(self.file_urls_frame, text=file.name).grid(row=row, column=0, sticky=tk.W)
                    entry = ttk.Entry(self.file_urls_frame, width=40)
                    entry.grid(row=row, column=1, sticky=(tk.W, tk.E), padx=5)
                    self.file_url_entries[file.name] = entry
                    row += 1

    def hide_file_urls(self):
        """Hide individual file URL entries"""
        for widget in self.file_urls_frame.winfo_children():
            widget.destroy()
        self.file_url_entries.clear()

    def on_select(self, event):
        """Handle tree item selection"""
        selected_item = self.tree.selection()[0]
        item_path = Path(self.tree.item(selected_item)['values'][0])

        # Reset configuration panel
        self.media_type_var.set('standalone')
        self.main_url_entry.delete(0, tk.END)
        self.collection_id_entry.delete(0, tk.END)
        self.collection_title_entry.delete(0, tk.END)
        self.collection_description_text.delete('1.0', tk.END)
        self.hide_file_urls()

        # Configure based on selection
        if item_path.is_file():
            # Disable inappropriate options for standalone files
            for child in self.config_frame.winfo_children():
                if isinstance(child, ttk.Radiobutton) and child['text'] in ['Movie Collection', 'TV Series']:
                    child.state(['disabled'])
                else:
                    child.state(['!disabled'])
            self.collection_frame.grid_remove()
            self.episode_urls_check.grid_remove()
        else:
            # Enable all options for folders
            # Enable all options for folders
            for child in self.config_frame.winfo_children():
                if isinstance(child, ttk.Radiobutton):
                    child.state(['!disabled'])
            # Show episode checkbox only for TV series
            self.media_type_var.trace_add('write', self.on_media_type_change)

    def process_media(self):
        """Process the selected media file/folder"""
        if not self.tree.selection():
            messagebox.showwarning("Warning", "Please select a media file or folder to process")
            return

        try:
            # Get selected item
            selected_item = self.tree.selection()[0]
            item_path = Path(self.tree.item(selected_item)['values'][0])

            # Create temporary media_references.json for selected item
            references = {}

            if item_path.is_file():
                # Standalone movie
                references = {
                    "type": "standalone_movie",
                    "url": self.main_url_entry.get(),
                    "filepath": item_path
                }
            else:
                # Folder - collection or series
                if self.media_type_var.get() == 'collection':
                    # Validate collection details
                    collection_id = self.collection_id_entry.get().strip()
                    collection_title = self.collection_title_entry.get().strip()
                    collection_description = self.collection_description_text.get('1.0', tk.END).strip()

                    if not collection_id or not collection_title:
                        messagebox.showerror("Error", "Collection ID and Title are required!")
                        return

                    references = {
                        "type": "movie_collection",
                        "ID": collection_id,
                        "collection_title": collection_title,
                        "collection_description": collection_description,
                        "movies": {
                            file_name: entry.get()
                            for file_name, entry in self.file_url_entries.items()

                        },
                        "filepath":item_path
                    }
                else:  # TV series
                    series_data = {
                        "type": "serie",
                        "collection_url": self.main_url_entry.get(),
                        "filepath": item_path
                    }
                    if self.individual_episodes_var.get():
                        series_data["episodes_url"] = [
                            entry.get()
                            for entry in self.file_url_entries.values()
                        ]
                    references = series_data



            # Update status and progress
            self.status_var.set("Processing media metadata...")
            self.progress['value'] = 25
            self.root.update()

            # Process the media
            collection, items = process_media_directory(self.covers_dir_var.get(), self.media_dir_var.get(), references, self.api_key_var.get(), 'http://www.omdbapi.com/')

            self.status_var.set("Adding to database...")
            self.progress['value'] = 50
            self.root.update()

            # Add to database
            process_data(self.database_var.get(), collection, items)

            self.status_var.set("Creating streaming files...")
            self.progress['value'] = 75
            self.root.update()

            # Create streaming files
            chunkonize(os.path.join(self.chunks_dir_var.get(), self.chunks_dir_var.get()), collection, items)

            self.status_var.set("Processing complete!")
            self.progress['value'] = 100
            self.root.update()

            messagebox.showinfo("Success", "Media processing completed successfully!")



        except Exception as e:
            messagebox.showerror("Error", f"Failed to process media: {str(e)}")
            self.status_var.set("Processing failed!")
        finally:
            self.progress['value'] = 0

if __name__ == "__main__":
    root = tk.Tk()
    app = MediaProcessingGUI(root)
    root.mainloop()