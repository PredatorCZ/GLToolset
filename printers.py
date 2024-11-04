from gdb.printing import PrettyPrinter, register_pretty_printer
import gdb

class LocalArray_char_Printer():
    """Print 'struct LocalArray<char>' as string"""

    def __init__(self, val):
        self.val: gdb.Value = val

    def to_string(self):
        start_addr = int(self.val["pointer"].address) + self.val["pointer"]
        inferior = gdb.inferiors()[0]
        bytes = inferior.read_memory(start_addr, self.val["numItems"]).tobytes()
        return '0x{:8x} "{}"'.format(int(start_addr), bytes.decode())

class PgPointer_Printer():
    """Print 'struct PlayGroung::Pointer<C>' as C*"""

    def __init__(self, val):
        self.val: gdb.Value = val
        self.ptr_type = str(val.type)[34:-1]
        self.ptr_type = gdb.lookup_type(self.ptr_type).pointer()
        owner_data = self.val["owner"]["data"]["_M_dataplus"]["_M_p"]
        start_addr = int(owner_data) + self.val["offset"]
        self.casted = start_addr.reinterpret_cast(self.ptr_type)

    def children(self):
        yield ('_raw', self.val["offset"])
        yield ('ptr', self.casted)

    def to_string(self):
        return '{...}'

class LocalArray_Printer():
    """Print 'struct LocalArray<C, D>' as {}"""

    def __init__(self, val, typename):
        self.val: gdb.Value = val
        self.numItems = val["numItems"]
        self.start_addr = int(self.val["pointer"].address) + self.val["pointer"]
        self.ptr_type = typename[26:typename.rfind(',')]
        self.ptr_type = gdb.lookup_type(self.ptr_type)
        self.sizeof = self.ptr_type.sizeof
        self.ptr_type = self.ptr_type.pointer()

    def children(self):
        for i in range(self.numItems):
            yield ('[{}]'.format(i), (self.start_addr + self.sizeof * i).cast(self.ptr_type))

    def to_string(self):
        return '{...}'

class LocalVariantPointer_Printer():
    """Print 'struct LocalVariantPointer<C...>' as C*"""

    def __init__(self, val):
        self.val: gdb.Value = val
        self.not_null = int(self.val["pointer"]) != 0
        self.start_addr = int(self.val["pointer"].address) + self.val["pointer"]
        self.ptr_types = str(val.type)
        self.ptr_types = self.ptr_types[35:-1].split(',')
        self.ptr_types = [gdb.lookup_type(i).pointer() for i in self.ptr_types]

    def children(self):
        if self.not_null:
            yield ('_raw', self.val["pointer"])
            yield ('value', self.start_addr.cast(self.ptr_types[0]))

    def to_string(self):
        return '{...}'

class LocalPointer_Printer():
    """Print 'struct LocalPointer<C>' as C*"""

    def __init__(self, val):
        self.val: gdb.Value = val
        self.not_null = int(self.val["pointer"]) != 0
        self.start_addr = int(self.val["pointer"].address) + self.val["pointer"]
        self.ptr_type = str(val.type)
        self.ptr_type = self.ptr_type[28:-1]
        self.ptr_type = gdb.lookup_type(self.ptr_type).pointer()

    def children(self):
        if self.not_null:
            yield ('_raw', self.val["pointer"])
            yield ('value', self.start_addr.cast(self.ptr_type))

    def to_string(self):
        return '{...}'

class CustomPrettyPrinterLocator(PrettyPrinter):
    """Given a gdb.Value, search for a custom pretty printer"""

    def __init__(self):
        super(CustomPrettyPrinterLocator, self).__init__(
            "prime_pretty_printers", []
        )

    def __call__(self, val):
        """Return the custom formatter if the type can be handled"""

        typename = gdb.types.get_basic_type(val.type).tag
        if typename is None:
            typename = val.type.name

        if not typename:
            return
        if typename.startswith('prime::common::LocalArray<char'):
            return LocalArray_char_Printer(val)
        elif typename.startswith('prime::utils::PlayGround::Pointer<'):
            return PgPointer_Printer(val)
        elif typename.startswith('prime::common::LocalArray<'):
            return LocalArray_Printer(val, typename)
        elif typename.startswith('prime::common::LocalVariantPointer<'):
            return LocalVariantPointer_Printer(val)
        elif typename.startswith('prime::common::LocalPointer<'):
            return LocalPointer_Printer(val)

register_pretty_printer(None, CustomPrettyPrinterLocator(), replace=True)
