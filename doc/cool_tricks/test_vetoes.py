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
from karabo.middlelayer import *
m1 = 'DETLAB_DSSC2/DET/0CH0:output'
m2 = 'DETLAB_DSSC2/DET/1CH0:output'
m3 = 'DETLAB_DSSC2/DET/2CH0:output'
m4 = 'DETLAB_DSSC2/DET/3CH0:output'

m = {
    "DETLAB_DSSC2/DET/0CH0:xtdf": "M1",
    "DETLAB_DSSC2/DET/1CH0:xtdf": "M2",
    "DETLAB_DSSC2/DET/2CH0:xtdf": "M3",
    "DETLAB_DSSC2/DET/3CH0:xtdf": "M4",
}

data = None
meta = None

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
    global i, data, meta

    #if not _meta.source.endswith('output'):
    #    return

    data = _data
    meta = _meta
    source = m[meta.source]

    # print(meta.timestamp.tid, meta.source)
    d = data['detector.data']

    ppt_veto = d[0] + d[1] * 256  # 2 uint8_t to uint16_t
    
    avetos = []
    for i in range(0, 16):
        asic_veto = d[162 + i*16] + (d[163 + i*16] * 256)
        avetos.append(asic_veto)
    print(source, ppt_veto, avetos)

    i += 1

def input_eos(channel):
    print('EOS', channel)

def input_close(channel):
    print('CLOSE', channel)

device_name, property_path = m1.split(":")
dev1 = getDevice(device_name)
prop1 = getattr(dev1, property_path)
prop1.setDataHandler(input_data)
prop1.connect()

device_name, property_path = m2.split(":")
dev2 = getDevice(device_name)
prop2 = getattr(dev2, property_path)
prop2.setDataHandler(input_data)
prop2.connect()

device_name, property_path = m3.split(":")
dev3 = getDevice(device_name)
prop3 = getattr(dev3, property_path)
prop3.setDataHandler(input_data)
prop3.connect()

device_name, property_path = m4.split(":")
dev4 = getDevice(device_name)
prop4 = getattr(dev4, property_path)
prop4.setDataHandler(input_data)
prop4.connect()
