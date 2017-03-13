﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;

namespace Sodium.Tests.Performance
{
    internal class Program
    {
        public static void Main3(string[] args)
        {
            DiscreteCellSink<bool> c = new DiscreteCellSink<bool>(false);

            Console.WriteLine("Press any key");
            Console.ReadKey();

            ((Action) (() =>
            {
                List<DiscreteCell<bool>> cc = new List<DiscreteCell<bool>>();
                for (int i = 0; i < 5000; i++)
                {
                    cc.Add(c.Map(v => !v));
                }

                Console.WriteLine("Press any key");
                Console.ReadKey();
            }))();

            Console.WriteLine("Press any key");
            Console.ReadKey();
        }

        public static void Main2(string[] args)
        {
            Console.WriteLine("Press any key");
            Console.ReadKey();

            //DiscreteCellSink<IReadOnlyList<SmallTestObject>> s = ((Func<DiscreteCellSink<IReadOnlyList<SmallTestObject>>>)(() =>
            //   new DiscreteCellSink<IReadOnlyList<SmallTestObject>>(new SmallTestObject[0])))();
            DiscreteCellSink<IReadOnlyList<SmallTestObject>> s = ((Func<DiscreteCellSink<IReadOnlyList<SmallTestObject>>>) (() =>
                new DiscreteCellSink<IReadOnlyList<SmallTestObject>>(Enumerable.Range(0, 500).Select(_ => new SmallTestObject()).ToArray())))();
            DiscreteCell<IReadOnlyList<bool>> s2 = s.Map(oo => oo.Select(o => o.S).Lift()).SwitchC();

            ((Action)(() =>
            {
                for (int i = 0; i < 5; i++)
                {
                    s.Send(Enumerable.Range(0, 500).Select(_ => new SmallTestObject()).ToArray());
                }
            }))();
            s.Send(new SmallTestObject[0]);

            Console.WriteLine("Press any key");
            Console.ReadKey();

            ((Action)(() =>
            {
                for (int i = 0; i < 5; i++)
                {
                    s.Send(Enumerable.Range(0, 500).Select(_ => new SmallTestObject()).ToArray());
                }
            }))();

            s.Send(new SmallTestObject[0]);

            Console.WriteLine("Press any key");
            Console.ReadKey();
        }

        private class SmallTestObject
        {
            public SmallTestObject()
            {
                this.S = new DiscreteCellSink<bool>(false);
            }

            public DiscreteCellSink<bool> S { get; }
        }

        public static void Main(string[] args)
        {
            Console.WriteLine("Press any key");
            Console.ReadKey();

            var t = Transaction.Run(() =>
            {
                CellLoop<bool?> allSelectedCellLoop = Cell.CreateLoop<bool?>();
                StreamSink<Unit> toggleAllSelectedStream = Stream.CreateSink<Unit>();
                Stream<bool> selectAllStream = toggleAllSelectedStream.Snapshot(allSelectedCellLoop).Map(a => a != true);

                IReadOnlyList<TestObject> o2 = Enumerable.Range(0, 10000).Select(n => new TestObject(n, selectAllStream)).ToArray();
                DiscreteCellSink<IReadOnlyList<TestObject>> objects =
                    DiscreteCell.CreateSink((IReadOnlyList<TestObject>)new TestObject[0]);

                var objectsAndIsSelected = objects.Map(oo => oo.Select(o => o.IsSelected.Map(s => new { Object = o, IsSelected = s })).Lift()).SwitchC();

                bool defaultValue = o2.Count < 1;
                DiscreteCell<bool?> allSelected =
                    objectsAndIsSelected.Map(
                        oo =>
                            !oo.Any()
                                ? defaultValue
                                : (oo.All(o => o.IsSelected)
                                    ? true
                                    : (oo.All(o => !o.IsSelected) ? (bool?)false : null)));
                allSelectedCellLoop.Loop(allSelected.Cell);

                return Tuple.Create(toggleAllSelectedStream, objectsAndIsSelected, selectAllStream, objects);
            });

            // ReSharper disable once UnusedVariable
            IListener l = Transaction.Run(() => t.Item2.Map(oo => oo.Count(o => o.IsSelected)).Updates.Listen(v => Console.WriteLine($"{v} selected")));

            Console.WriteLine("Press any key");
            Console.ReadKey();

            Stopwatch sw = new Stopwatch();
            sw.Start();

            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            Thread.Sleep(500);
            SendMore(t.Item4, t.Item3);
            t.Item4.Cell.Sample()[2].IsSelectedStreamSink.Send(true);
            Transaction.RunVoid(() =>
            {
                t.Item4.Cell.Sample()[3].IsSelectedStreamSink.Send(true);
                t.Item4.Cell.Sample()[4].IsSelectedStreamSink.Send(true);
            });
            Transaction.RunVoid(() =>
            {
                t.Item4.Send(Enumerable.Range(0, 2500).Select(n => new TestObject(n, t.Item3)).ToArray());
                t.Item1.Send(Unit.Value);
            });
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item1.Send(Unit.Value);
            t.Item4.Send(new TestObject[0]);

            sw.Stop();

            Console.WriteLine();
            Console.WriteLine();
            Console.WriteLine($"Elapsed: {sw.ElapsedMilliseconds}ms");

            Console.WriteLine();
            Console.WriteLine("Press any key");
            Console.ReadKey();
        }

        private static void SendMore(DiscreteCellSink<IReadOnlyList<TestObject>> cellSink, Stream<bool> selectAllStream)
        {
            cellSink.Send(Enumerable.Range(0, 20000).Select(n => new TestObject(n, selectAllStream)).ToArray());
        }

        private class TestObject
        {
            public TestObject(int id, Stream<bool> selectAllStream)
            {
                this.Id = id;
                this.IsSelectedStreamSink = Stream.CreateSink<bool>();
                this.IsSelected = selectAllStream.OrElse(this.IsSelectedStreamSink).Hold(false);
            }

            public int Id { get; }
            public StreamSink<bool> IsSelectedStreamSink { get; }
            public DiscreteCell<bool> IsSelected { get; }
        }
    }
}
