from conans import ConanFile

class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    generators = 'cmake_find_package'

    requires = [
        ('boost/1.78.0'),
    ]

    default_options = (
        'boost:shared=False',
        'boost:without_math=True',
        'boost:without_graph=True',
        'boost:without_stacktrace=True',
        'boost:without_fiber=True'
    )