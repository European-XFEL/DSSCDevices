# This script can be used from ikarabo like so:
#
#     > ikarabo
#     In [1]: import test_vetoes
#
# You will then see the temperature for each ASICs for each specified ladder
# The ASIC temperature field found in the XTDF trailer data is actually abused
# to show the preveto as processed by each ASIC.
# The preveto sent from the PPT to the IOB is also present.
# We can thus compare the two to check for unresponsive ASICs
# If you see a not-matching value (namely, 0), then the ASIC is not sending data
# N.B.: the temperature is actually 9 bits, whereas the preveto is 32bits.
# so you'll get overflow for prevetos larger than 511, which is fine because
# who's crazy enough to have such preveto, what the hell are ya doing to my baby?
import numpy as np
from karabo.middlelayer import *
q1m1 = 'SCS_DET_DSSC2/DET/0CH0:output'
q1m2 = 'SCS_DET_DSSC2/DET/1CH0:output'
q1m3 = 'SCS_DET_DSSC2/DET/2CH0:output'
q1m4 = 'SCS_DET_DSSC2/DET/3CH0:output'

q3m1 = 'SCS_DET_DSSC2/DET/8CH0:output'
q3m2 = 'SCS_DET_DSSC2/DET/9CH0:output'
q3m3 = 'SCS_DET_DSSC2/DET/10CH0:output'
q3m4 = 'SCS_DET_DSSC2/DET/11CH0:output'

q4m1 = 'SCS_DET_DSSC2/DET/12CH0:output'
q4m2 = 'SCS_DET_DSSC2/DET/13CH0:output'
q4m3 = 'SCS_DET_DSSC2/DET/14CH0:output'
q4m4 = 'SCS_DET_DSSC2/DET/15CH0:output'

m = {
    "SCS_DET_DSSC2/DET/0CH0:xtdf": "Q1M1",
    "SCS_DET_DSSC2/DET/1CH0:xtdf": "Q1M2",
    "SCS_DET_DSSC2/DET/2CH0:xtdf": "Q1M3",
    "SCS_DET_DSSC2/DET/3CH0:xtdf": "Q1M4",

    "SCS_DET_DSSC2/DET/8CH0:xtdf": "Q3M1",
    "SCS_DET_DSSC2/DET/9CH0:xtdf": "Q3M2",
    "SCS_DET_DSSC2/DET/10CH0:xtdf": "Q3M3",
    "SCS_DET_DSSC2/DET/11CH0:xtdf": "Q3M4",

    "SCS_DET_DSSC2/DET/12CH0:xtdf": "Q4M1",
    "SCS_DET_DSSC2/DET/13CH0:xtdf": "Q4M2",
    "SCS_DET_DSSC2/DET/14CH0:xtdf": "Q4M3",
    "SCS_DET_DSSC2/DET/15CH0:xtdf": "Q4M4",
}

i = 0
data = None
meta = None
previous_tid = None

def get_shmem_array(value):
    try:
        name, dtype, shape, index = value.split('$')
        shape = tuple((int(x) for x in shape.split(',')))
        index = int(index)
    except ValueError:
        return value

    try:
        import posixshmem
    except ImportError:
        # No shared memory support.
        return value

    prev_name = getattr(self, '_shmem_name', None)

    if prev_name != name:
        if prev_name is not None:
            del self._shmem_array
            del self._shmem_handle

        size = np.dtype(dtype).itemsize * np.product(shape)

        try:
            self._shmem_handle = posixshmem.SharedMemory(
                name=name, size=size, rw=False)
        except Exception:
            # For whatever reason it fails, just return the value.
            return value

        self._shmem_name = name
        self._shmem_array = self._shmem_handle.ndarray(
            shape=shape, dtype=dtype)

    return self._shmem_array[index]


def input_data(_data, _meta):
    global i, data, meta, previous_tid

    #if not _meta.source.endswith('output'):
    #    return

    data = _data
    meta = _meta
    source = m[meta.source]

    tid = meta.timestamp.timestamp.tid
    printed = False

    if tid != previous_tid:
        previous_tid = tid
        print(flush=True)


    # print(meta.timestamp.tid, meta.source)
    d = data['detector.data']

    det_cell_id = get_array_data(data, path="image.cellId").squeeze()

    ppt_veto = d[0] + d[1] * 256  # 2 uint8_t to uint16_t
    
    avetos = []
    for i in range(0, 16):
        asic_veto = d[162 + i*16] + (d[163 + i*16] * 256)
        avetos.append(asic_veto)

    something = False
    avetos_string = "["
    for veto in avetos:
        if veto != ppt_veto:
            avetos_string += f"\033[91m{str(veto).rjust(3, ' ')}\033[0m"
            something = True
        else:
            avetos_string += str(veto).rjust(3, " ")
        avetos_string += ", "
    avetos_string = avetos_string[:-2] + "]"

    if something:
        print(tid, source, ppt_veto, avetos_string, end=" ")
        printed = True

    if np.any(det_cell_id[det_cell_id > 800]):
        print("CELL ID OVER 800", end="")
        printed = True

    if printed:
        print()


    i += 1

def input_eos(channel):
    print('EOS', channel)

def input_close(channel):
    print('CLOSE', channel)

device_name, property_path = q1m1.split(":")
dev1 = getDevice(device_name)
prop1 = getattr(dev1, property_path)
prop1.setDataHandler(input_data)
prop1.connect()

device_name, property_path = q1m2.split(":")
dev2 = getDevice(device_name)
prop2 = getattr(dev2, property_path)
prop2.setDataHandler(input_data)
prop2.connect()

device_name, property_path = q1m3.split(":")
dev3 = getDevice(device_name)
prop3 = getattr(dev3, property_path)
prop3.setDataHandler(input_data)
prop3.connect()

#device_name, property_path = q1m4.split(":")
#dev4 = getDevice(device_name)
#prop4 = getattr(dev4, property_path)
#prop4.setDataHandler(input_data)
#prop4.connect()

device_name, property_path = q3m1.split(":")
dev5 = getDevice(device_name)
prop5 = getattr(dev5, property_path)
prop5.setDataHandler(input_data)
prop5.connect()

device_name, property_path = q3m2.split(":")
dev6 = getDevice(device_name)
prop6 = getattr(dev6, property_path)
prop6.setDataHandler(input_data)
prop6.connect()

device_name, property_path = q3m3.split(":")
dev7 = getDevice(device_name)
prop7 = getattr(dev7, property_path)
prop7.setDataHandler(input_data)
prop7.connect()

device_name, property_path = q3m4.split(":")
dev8 = getDevice(device_name)
prop8 = getattr(dev8, property_path)
prop8.setDataHandler(input_data)
prop8.connect()

# device_name, property_path = q4m1.split(":")
# dev9 = getDevice(device_name)
# prop9 = getattr(dev9, property_path)
# prop9.setDataHandler(input_data)
# prop9.connect()

device_name, property_path = q4m2.split(":")
dev10 = getDevice(device_name)
prop10 = getattr(dev10, property_path)
prop10.setDataHandler(input_data)
prop10.connect()

device_name, property_path = q4m3.split(":")
dev11 = getDevice(device_name)
prop11 = getattr(dev11, property_path)
prop11.setDataHandler(input_data)
prop11.connect()

device_name, property_path = q4m4.split(":")
dev12 = getDevice(device_name)
prop12 = getattr(dev12, property_path)
prop12.setDataHandler(input_data)
prop12.connect()