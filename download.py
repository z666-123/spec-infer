# Use a pipeline as a high-level helper
from transformers import pipeline

pipe = pipeline("text-generation", model="JackFram/llama-68m")
