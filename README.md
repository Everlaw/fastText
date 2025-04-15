# fasttext-predict

Python package for [fasttext](https://github.com/facebookresearch/fastText):

* keep only the `predict` method, all other features are removed
* standalone package without external dependency (numpy is not a dependency)
* wheels for various architectures using GitHub workflows. The script is inspired by lxml build scripts.

## Usage

```sh
pip install fasttext-predict
```

```python
import fasttext
model = fasttext.load_model('lid.176.ftz')
result = model.predict('Fondant au chocolat et tarte aux myrtilles')
```

See https://fasttext.cc/docs/en/language-identification.html
