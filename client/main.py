import sys
from ui import App
from PyQt5.Qt import QApplication


def main():
    app = QApplication(sys.argv)
    ex = App()
    app.exec()


if __name__ == '__main__':
    main()
