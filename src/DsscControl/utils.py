from typing import List, Tuple, Union

import numpy as np
from karabo.middlelayer import Hash, call, getConfiguration, getSchema, setWait


async def sanitize_r(path, k, v, shash, da):
    minsize = None

    try:
        minsize = shash[path].getAttribute(k, 'minSize')
    except KeyError:
        pass
    if minsize and len(v) != minsize:
        dval = None
        if isinstance(v, np.ndarray):
            dval = np.zeros(minsize, v.dtype)
        if isinstance(v, bytes):
            dval = bytearray(minsize)
        if dval is not None:
            # bug in Karabo prevent setting on minSize > 0, temporarily set 0
            # TODO: still the case?
            upH = Hash()
            upH["path"] = "{}.{}".format(path, k)
            upH["attribute"] = "minSize"
            upH["value"] = 0
            await call(da, "slotUpdateSchemaAttributes", [upH])
            # not set the new default which is within bounds
            await setWait(da, "{}.{}".format(path, k), dval)
            # set back to actual value
            upH["value"] = minsize
            await call(da, "slotUpdateSchemaAttributes", [upH])


async def sanitize_da(da, bp='xtdf.schema'):
    schema = await getSchema(da)
    shash = schema.hash
    cconf = await getConfiguration(da)
    for k, v in cconf[bp].items():
        if isinstance(v, Hash):
            await sanitize_da(da, "{}.{}".format(bp, k))
        else:
            await sanitize_r(bp, k, v, shash, da)


def strToByteArray(str):
    byteArray = [np.uint8(ord(s)) for s in str]
    byteArray.append(np.uint8(0))
    return byteArray


def get_sweep_vector(user_input: str) -> Tuple[bool, Union[str, List[int]]]:
    try:
        sequence = eval(user_input)
        sequence = list(sequence)  # Handle expressions such as range()
        assert isinstance(sequence, list)
        assert all(isinstance(v, int) for v in sequence)
    except (AssertionError, NameError, SyntaxError, TypeError):
        return False, "Invalid Expression"

    return True, sequence


class MeasurementConfig:
    configFileName = ''
    numIterations = 1
    numPreBurstVetos = 0
    ladderMode = 1
    activeASIC = 0
