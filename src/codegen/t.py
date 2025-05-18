import nest_asyncio

nest_asyncio.apply()

# NOTE: the code examples assume you're operating within a Jupyter notebook.
# download files
!mkdir data
!wget "https://www.dropbox.com/s/948jr9cfs7fgj99/UBER.zip?dl=1" -O data/UBER.zip
!unzip data/UBER.zip -d data

from llama_index.readers.file import UnstructuredReader
from pathlib import Path

years = [2022, 2021, 2020, 2019]

loader = UnstructuredReader()
doc_set = {}
all_docs = []
for year in years:
    year_docs = loader.load_data(
        file=Path(f"./data/UBER/UBER_{year}.html"), split_documents=False
    )
    # insert year metadata into each year
    for d in year_docs:
        d.metadata = {"year": year}
    doc_set[year] = year_docs

    from llama_index.embeddings.huggingface import HuggingFaceEmbedding
from llama_index.core import Settings

# Set a local embedding model
Settings.embed_model = HuggingFaceEmbedding(model_name="thenlper/gte-small")

# Now your original code
index_set = {}
for year in years:
    storage_context = StorageContext.from_defaults()
    cur_index = VectorStoreIndex.from_documents(
        doc_set[year],
        storage_context=storage_context,
    )
    index_set[year] = cur_index
    storage_context.persist(persist_dir=f"./storage/{year}")

    # Load indices from disk
from llama_index.core import load_index_from_storage

index_set = {}
for year in years:
    storage_context = StorageContext.from_defaults(
        persist_dir=f"./storage/{year}"
    )
    cur_index = load_index_from_storage(
        storage_context,
    )
    index_set[year] = cur_index

    from llama_index.llms.huggingface import HuggingFaceLLM
from transformers import AutoModelForCausalLM, AutoTokenizer
import torch

# Load TinyLlama locally
model_name = "TinyLlama/TinyLlama-1.1B-Chat-v1.0"
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name, device_map="auto", torch_dtype=torch.float16)

# Set TinyLlama as the default LLM for querying
from llama_index.core import Settings
Settings.llm = HuggingFaceLLM(
    model=model,
    tokenizer=tokenizer,
    context_window=2048,
    max_new_tokens=512,
    generate_kwargs={"temperature": 0.0},
    tokenizer_kwargs={"padding_side": "left", "truncation_side": "left"},
)


from llama_index.core.tools import QueryEngineTool, ToolMetadata

individual_query_engine_tools = [
    QueryEngineTool(
        query_engine=index_set[year].as_query_engine(),  # Now uses TinyLlama or None
        metadata=ToolMetadata(
            name=f"vector_index_{year}",
            description=f"useful for answering for when you want to answer queries about the {year} SEC 10-K for Uber",
        ),
    )
    for year in years
]

from llama_index.core.query_engine import SubQuestionQueryEngine
from llama_index.llms.huggingface import HuggingFaceLLM
from transformers import AutoModelForCausalLM, AutoTokenizer
import torch

# 1. Load a local model
model_name = "TinyLlama/TinyLlama-1.1B-Chat-v1.0"
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name, device_map="auto", torch_dtype=torch.float16)

# 2. Create a HuggingFaceLLM
local_llm = HuggingFaceLLM(
    model=model,
    tokenizer=tokenizer,
    context_window=2048,
    max_new_tokens=512,
    generate_kwargs={"temperature": 0.0},
    tokenizer_kwargs={"padding_side": "left", "truncation_side": "left"},
)

query_engine = SubQuestionQueryEngine.from_defaults(
    query_engine_tools=individual_query_engine_tools,
    llm=local_llm,  # <<< Use your Huggingface TinyLlama instead of OpenAI
)

from llama_index.core.tools import QueryEngineTool, ToolMetadata

# Wrap the SubQuestionQueryEngine into a QueryEngineTool
query_engine_tool = QueryEngineTool(
    query_engine=query_engine,  # your SubQuestionQueryEngine object
    metadata=ToolMetadata(
        name="sub_question_query_engine",
        description="Useful for answering complex queries that require analyzing multiple SEC 10-K documents for Uber.",
    ),
)

tools = individual_query_engine_tools + [query_engine_tool]

from llama_index.core.agent.workflow import FunctionAgent

from llama_index.llms.huggingface import HuggingFaceLLM



agent = FunctionAgent(
    tools=tools,
    llm=local_llm,
)

async def main():
    response = await agent.run("hi, i am bob")
    print(str(response))

# In Colab, you can just:
await main()


Response: ValueError                                Traceback (most recent call last)
/usr/local/lib/python3.11/dist-packages/llama_index/core/workflow/context.py in _step_worker(self, name, step, config, stepwise, verbose, checkpoint_callback, run_id, service_manager, dispatcher)
    582                     try:
--> 583                         new_ev = await instrumented_step(**kwargs)
    584                         kwargs.clear()

8 frames
ValueError: LLM must be a FunctionCallingLLM

The above exception was the direct cause of the following exception:

WorkflowRuntimeError                      Traceback (most recent call last)
/usr/local/lib/python3.11/dist-packages/llama_index/core/workflow/context.py in _step_worker(self, name, step, config, stepwise, verbose, checkpoint_callback, run_id, service_manager, dispatcher)
    590                     except Exception as e:
    591                         if config.retry_policy is None:
--> 592                             raise WorkflowRuntimeError(
    593                                 f"Error in step '{name}': {e!s}"
    594                             ) from e

WorkflowRuntimeError: Error in step 'run_agent_step': LLM must be a FunctionCallingLLM